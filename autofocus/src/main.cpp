#define USE_OPENCV

#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif
#include <libv4l2.h>

#include <libv4l2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <string>
#include <set>

#include <signal.h>


#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <errno.h>
#include <vector>
#include <cstring>
#include <cassert>

#include "v4l2.hpp"
#include "ipc.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <boost/timer/timer.hpp>


#include <array>

#include "libv4lconvert.h"

#include "BufferReference.h"

#include "FocusAnalyser.h"

void fillWithLumaFromYUY2(uint8_t yuy2[],size_t width,size_t height,uint8_t luma[]) {
	for (size_t row = 0; row < height; row++) {
		for (size_t column = 0; column < width; column++) {
			luma[row*width+column] = yuy2[row*width*2+column*2+1];

		}
	}
}


struct FrameBuffer {
	void* pointer;
	size_t width;
	size_t height;
};




int getInitialDirection(int focusValue) {
	return focusValue < 192 ? 255 : 0;
}


sig_atomic_t volatile running = 1;

void termination_handler(int signal) {
	running = 0;
}


int main() {
	struct sigaction termination;
	memset(&termination, 0, sizeof(struct sigaction));
	termination.sa_handler = &termination_handler;
	sigemptyset(&termination.sa_mask);
	termination.sa_flags = 0;
	sigaction(SIGTERM, &termination, NULL);


    int deviceDescriptor = open ("/dev/video0", O_RDWR /* required */ | O_NONBLOCK, 0);
    if (deviceDescriptor == -1) {
    	std::cout << "Unable to open device";
    	return 1;
    }


#ifdef USE_OPENCV
    cv::namedWindow("input",1);
#endif


//	std::vector<uint8_t> iminput(width*height*3);

    int bpp = 1;

    bool isAutofocusAvailable = isLogitechAutofocusModeSupported(deviceDescriptor);




    if (isAutofocusAvailable) {
    }


    int announce_socket;
    in_addr iaddr;

    memset(&iaddr, 0, sizeof(struct in_addr));

    announce_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if ( announce_socket < 0 ) {
      perror("Error creating socket");
      exit(0);
	}

    sockaddr_in saddr;
    memset(&saddr, 0, sizeof(struct sockaddr_in));

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(CAMERA_ANNOUNCE_PORT);; // Use the first free port
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


	int released_frames_mq = mq_open("/video0-released-frames",
			O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, NULL);
	if (-1 == released_frames_mq) {
		printf("mq_open failed");
		return 1;
	}


	std::vector<FrameBuffer> buffers;

	mq_attr attr;
	if (-1 == mq_getattr(released_frames_mq,&attr)) {
		printf("mq_getattr failed");
		return 1;
	}
	unsigned priority;
	size_t count = 0;

	std::array<char,1024> mq_buffer;
	FocusAnalyser detector;
	timespec focus_change_time;
    set_focus_variable(deviceDescriptor,0);
    int iteration_number = 1;
    size_t indexOfMaximumFromZero;
	clock_gettime(CLOCK_MONOTONIC,&focus_change_time);
	bool cr_needed = false;

	while (running == 1) {
		BufferReference readyBuffer;
		memset(&readyBuffer,0,sizeof(readyBuffer));

			std::vector<char> data(attr.mq_msgsize+1);

			msghdr socket_message;
			memset(&socket_message,0,sizeof(socket_message));
			int result;
			{
//				boost::timer::auto_cpu_timer timer;
				result = recvfrom(announce_socket,data.data(),data.size(),0,NULL,NULL);
			}

			if (result <= 0) {
				perror("recvfrom");
				break;
			}
	    	timespec tp;
	    	clock_gettime(CLOCK_MONOTONIC,&tp);

			void* input_pointer = &readyBuffer;

			ber_decode(0,&asn_DEF_BufferReference,&input_pointer,data.data(),result);
			count++;

			if (readyBuffer.timestamp_seconds > focus_change_time.tv_sec || (readyBuffer.timestamp_seconds == focus_change_time.tv_sec && readyBuffer.timestamp_microseconds > (focus_change_time.tv_nsec + 500)/1000)) {
				if (cr_needed) {
					std::cout << "!" << std::endl;
					cr_needed = false;
				}
			}
			#ifdef USE_OPENCV
				cv::Mat output(readyBuffer.height,readyBuffer.width,CV_8UC3);
			#endif

			if (!cr_needed) {


				uint32_t index = readyBuffer.index;
		//    	printf("Index = %u, seconds = %ld ms = %ld\n", index,readyBuffer.timestamp_seconds,readyBuffer.timestamp_microseconds);
		//    	printf("Real time diff: Index = %u, seconds = %ld, us = %ld\n", index, tp.tv_sec - readyBuffer.timestamp_seconds,(1000 + tp.tv_nsec/1000 - readyBuffer.timestamp_microseconds)%1000);

				if (index >= buffers.size()) {
					buffers.resize(index+1);
				}

		//TODO Finalize and test the dynamic change of the frame resolution functionality
				FrameBuffer& savedBuffer = buffers[index];
				if (savedBuffer.pointer == nullptr || savedBuffer.width != readyBuffer.width || savedBuffer.height != readyBuffer.height) {
					if (savedBuffer.pointer != nullptr) {
						munmap (savedBuffer.pointer, savedBuffer.width*savedBuffer.height*2);
					}
					void* pointer = mmap (NULL, readyBuffer.width*readyBuffer.height*2,
								 PROT_READ,
								 MAP_SHARED,
								 deviceDescriptor, readyBuffer.offset);
					if (pointer == MAP_FAILED) {
						printf("mmap failed");
						return 1;
					}
					savedBuffer.pointer = pointer;
					savedBuffer.width = readyBuffer.width;
					savedBuffer.height = readyBuffer.height;
				}


				int err  = errno;
		#ifdef USE_OPENCV
				cv::Mat input(readyBuffer.height,readyBuffer.width,CV_8UC2,(uint8_t*)savedBuffer.pointer);
				uint32_t focus_sum = 0;
				for (uint32_t row = input.rows/3 +1; row < input.rows*2/3;row++) {
					for (uint32_t column = input.cols/3 +1; column < input.cols*2/3;column++) {
						focus_sum += abs((input.at<uint16_t>(cv::Point(column,row)) & 0xFF) - (input.at<uint16_t>(cv::Point(column,row-1)) & 0xFF));
						focus_sum += abs((input.at<uint16_t>(cv::Point(column,row)) & 0xFF) - (input.at<uint16_t>(cv::Point(column-1,row)) & 0xFF));
					}
				}
				detector.addFocusMeasurementResult(focus_sum);

				std::cout << focus_sum << ",";
				if (detector.isFocusStabilised()) {
					if (iteration_number >= 2 && iteration_number %2 == 0) {
						indexOfMaximumFromZero = detector.getMaximumValueIndex();
					}
					if (iteration_number >= 3 && iteration_number %2 != 0) {
						size_t indexOfMaximumFromMax = detector.getMaximumValueIndex();
		//				std::cout << "Indexes " << indexOfMaximumFromZero/indexOfMaximumFromMax << std::endl;
						std::cout << "Indexes " << indexOfMaximumFromZero << "-" << indexOfMaximumFromMax << std::endl;

					}
					iteration_number++;
					cr_needed = true;
					detector = FocusAnalyser();

					uint8_t focus = get_focus_variable(deviceDescriptor);
		//			if (focus == 255) {
		//				break;
		//			}
					set_focus_variable(deviceDescriptor,focus == 255 ? 0 : 255);
					clock_gettime(CLOCK_MONOTONIC,&focus_change_time);

		//		    set_focus_variable(focus +1,deviceDescriptor);
				}



				cv::cvtColor(input,output,CV_YUV2BGR_YUY2);
		#endif
		}

				asn_enc_rval_t encode_result = der_encode_to_buffer(&asn_DEF_BufferReference, &readyBuffer,mq_buffer.data(),mq_buffer.size());

				mq_send(released_frames_mq,mq_buffer.data(),encode_result.encoded,0);

		#ifdef USE_OPENCV
				cv::imshow("input", output);
		//		static bool done = false;
		//		static bool done2 = false;
		//		if (!done) {
		//			cv::imshow("input", output);
		//			done = true;
		//		} else {
		//			if (!done2) {
		//				cv::imshow("input2", output);
		//				done2 = true;
		//			}
		//		}
		#endif

//				struct v4l2_buffer buf;
//				memset(&buf,0,sizeof(buf));
//
//				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//				buf.index = index;

#ifdef USE_OPENCV
        if(int key = cv::waitKey(1) >= 0) {
        	break;
        }
#endif
	}
	std::cout << "count:" << count << std::endl;



	for (FrameBuffer buffer: buffers) {
		if (buffer.pointer != nullptr) {
			munmap (buffer.pointer, buffer.width*buffer.height*2);
		}
	}
	close(announce_socket);

	return 0;
}
