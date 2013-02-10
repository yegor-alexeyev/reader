
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <fcntl.h>

#include <string>

#include <iostream>
#include <sys/mman.h>
#include <errno.h>
#include <vector>
#include <cstring>
#include <cassert>

#include "ipc.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <array>

#include "BufferReference.h"

#include "yuy2.h"


int getInitialDirection(int focusValue) {
	return focusValue < 192 ? 255 : 0;
}


int get_milliseconds_delta(timeval before, timespec after) {
	size_t before_msec = ((before.tv_usec+500)/1000);
	size_t after_msec = ((after.tv_nsec+500000)/1000000);

	return (after.tv_sec - before.tv_sec)*1000 + after_msec - before_msec;
}


int main() {
	initialise_termination_handler();

    cv::namedWindow("input",1);

    int announce_socket;
    in_addr iaddr;

    memset(&iaddr, 0, sizeof(struct in_addr));

    announce_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if ( announce_socket < 0 ) {
      perror("Error creating socket");
      exit(0);
	}


    int one = 1;
    int ret = setsockopt(announce_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (ret < 0) {
    	perror("setsockopt");
    	return 1;
    }


    sockaddr_in saddr;
    memset(&saddr, 0, sizeof(struct sockaddr_in));

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(CAMERA_ANNOUNCE_PORT);
//    saddr.sin_port = 0; // Use the first free port
    saddr.sin_addr.s_addr = INADDR_ANY; // bind socket to any interface
    int status = bind(announce_socket, (struct sockaddr *)&saddr, sizeof(sockaddr_in));

    if ( status < 0 )
      perror("Error binding socket to interface"), exit(0);


    ip_mreq mreq;
    memset(&mreq,0,sizeof(mreq));
    mreq.imr_multiaddr.s_addr=inet_addr(CAMERA_ANNOUNCE_GROUP);
    mreq.imr_interface.s_addr=INADDR_ANY;
    if (setsockopt(announce_socket,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
	  perror("setsockopt");
	  exit(1);
    }

	while (running == 1) {
		BufferReference readyBuffer;
		memset(&readyBuffer,0,sizeof(readyBuffer));

		msghdr socket_message;
		memset(&socket_message,0,sizeof(socket_message));
		int result;

		std::vector<char> data(1024);

		result = recvfrom(announce_socket,data.data(),data.size(),0,NULL,NULL);

		if (result <= 0) {
			perror("recvfrom");
			break;
		}

		void* input_pointer = &readyBuffer;

		ber_decode(0,&asn_DEF_BufferReference,&input_pointer,data.data(),result);

		std::string buffer_name = "/" + get_name_of_buffer(readyBuffer.sequence);
		int buffer_descriptor = shm_open(buffer_name.c_str(), O_RDONLY, 0);
		if (buffer_descriptor == -1) {
			std::cout << "Can not open shared buffer file " << buffer_name << std::endl;
			continue;
		}
		size_t buffer_length = readyBuffer.width*readyBuffer.height*2;
		void* pointer = mmap(NULL,buffer_length, PROT_READ,MAP_PRIVATE,buffer_descriptor,0);
		close(buffer_descriptor);
		if (pointer == MAP_FAILED) {
			std::cout << "Can not map shared buffer " << buffer_name << std::endl;
			continue;
		}

		yuy2::view_t frame = boost::gil::interleaved_view(readyBuffer.width,readyBuffer.height,static_cast<yuy2::ptr_t>(pointer),readyBuffer.width*2);

		cv::Mat input(readyBuffer.height,readyBuffer.width,CV_8UC2,(uint8_t*)pointer);
		cv::Mat output(readyBuffer.height,readyBuffer.width,CV_8UC3);

		cv::cvtColor(input,output,CV_YUV2BGR_YUY2);

		munmap(pointer,buffer_length);

		cv::imshow("input", output);

		timespec tp;
		clock_gettime(CLOCK_MONOTONIC,&tp);

		timeval frame_timestamp{readyBuffer.timestamp_seconds, readyBuffer.timestamp_microseconds};

		std::cout << "Visual delay = " << get_milliseconds_delta(frame_timestamp,tp) << " ms" << std::endl;

        if(int key = cv::waitKey(1) >= 0) {
        	break;
        }
	}

	close(announce_socket);

	return 0;
}
