#include <stdio.h>
#include <sys/stat.h>

#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <libv4l2.h>

#include <linux/videodev2.h>

#include <map>

#include <vector>
#include <assert.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "v4l2.hpp"
#include "ipc.hpp"

#include <array>
#include <iostream>

#include "BufferReference.h"

#include <boost/timer/timer.hpp>

#include <execinfo.h>

struct frame_buffer {
	size_t length;
	void* pointer;
};

const uint32_t CAMERA_FRAME_WIDTH = 640;
const uint32_t CAMERA_FRAME_HEIGHT = 480;

//
//std::string get_name_of_queued_buffer(__u32 buffer_index) {
//	return "video0-pending-" + std::to_string(buffer_index);
//}
//
//
//
//

std::map<unsigned long,size_t> ptrToSequenceMap;
size_t sequence = 0;


void* prepare_frame_buffer(__u32 buffer_length) {
	std::string name = get_name_of_buffer(sequence);
	int file_descriptor = shm_open(("/"+name).c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR );
	if (file_descriptor == -1) {
		throw std::runtime_error("shm_open fail. Unable to create " + name);
	}

	ftruncate(file_descriptor, buffer_length);
	void* mapping = mmap(NULL, buffer_length,PROT_READ | PROT_WRITE,MAP_SHARED,file_descriptor,0);
	if (mapping == MAP_FAILED) {
		throw std::runtime_error("Can not map shared buffer " + name);
	}

	close(file_descriptor);
	unsigned long userptr = reinterpret_cast<unsigned long>(mapping);
	assert(ptrToSequenceMap.count(userptr) == 0);
	ptrToSequenceMap[userptr] = sequence;
	sequence++;
	return mapping;
}

void queueNextFrameBuffer(int deviceDescriptor, __u32 buffer_index, uint32_t sequence_number, __u32 buffer_length) {

	void* mapping = prepare_frame_buffer(buffer_length);
	std::cout << "mapping: " << mapping << "length:" << buffer_length << std::endl;

	queue_frame_buffer(deviceDescriptor, buffer_index,mapping,buffer_length);
}




size_t start_capturing(int deviceDescriptor) {
	struct v4l2_requestbuffers reqbuf;

	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_USERPTR;
	reqbuf.count = 20;

	if (-1 == ioctl(deviceDescriptor, VIDIOC_REQBUFS, &reqbuf)) {
		if (errno == EINVAL)
			printf("Video capturing or userptr-streaming is not supported\n");
		else
			perror("VIDIOC_REQBUFS");

		exit(EXIT_FAILURE);
	}

	if (reqbuf.count < 5) {
		printf("Not enough buffer memory\n");
		exit(EXIT_FAILURE);
	}

	std::vector<frame_buffer> buffers(reqbuf.count);

	for (size_t i = 0; i < reqbuf.count; i++) {
		frame_buffer& mapped_buffer = buffers[i];

		mapped_buffer.length = CAMERA_FRAME_WIDTH*CAMERA_FRAME_HEIGHT*2;
		mapped_buffer.pointer = prepare_frame_buffer(mapped_buffer.length);
	}

	enum v4l2_buf_type type;
	for (size_t index = 0; index < buffers.size(); index++) {
		const frame_buffer& mapped_buffer = buffers[index];

		queue_frame_buffer(deviceDescriptor,index,mapped_buffer.pointer,mapped_buffer.length);
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(deviceDescriptor, VIDIOC_STREAMON, &type))
		perror("VIDIOC_STREAMON");
	return reqbuf.count;
}


sig_atomic_t volatile running = 1;

void termination_handler(int signal) {
	running = 0;
}

void initialise_termination_handler() {
	struct sigaction termination;
	memset(&termination, 0, sizeof(struct sigaction));
	termination.sa_handler = &termination_handler;
	sigemptyset(&termination.sa_mask);
	termination.sa_flags = 0;
	sigaction(SIGTERM, &termination, NULL);
}


void camera_server() {
	size_t count = 0;

	initialise_termination_handler();

	int deviceDescriptor = v4l2_open("/dev/video0",
			O_RDWR /* required */| O_NONBLOCK, 0);
	if (deviceDescriptor == -1) {
		throw std::runtime_error("Unable to open device");
	}

//	disable_output_processing(deviceDescriptor);

	if (!isStreamingIOSupported(deviceDescriptor)) {
		throw std::runtime_error("Streaming is not supported");
	}


	setCameraOutputFormat(deviceDescriptor, CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT, V4L2_PIX_FMT_YUYV);


	std::cout << "Absolute focus supported: " << isControlSupported(deviceDescriptor,V4L2_CID_FOCUS_ABSOLUTE) << std::endl;
	std::cout << "Relative focus supported: " << isControlSupported(deviceDescriptor,V4L2_CID_FOCUS_RELATIVE) << std::endl;

//	set_exposure_auto_priority(deviceDescriptor,true);
//	printf("Is exposure auto priority set = %u\n", is_exposure_auto_priority(deviceDescriptor));
//
	set_manual_exposure(deviceDescriptor,true);
	printf("Is manual exposure set = %u\n", is_manual_exposure(deviceDescriptor));
	set_absolute_exposure(30*10,deviceDescriptor);

	set_auto_white_balance(deviceDescriptor,false);
	printf("Is auto white balance set = %u\n", is_auto_white_balance_set(deviceDescriptor));

	set_gain(deviceDescriptor,1);
	printf("Gain set = %u\n", get_gain(deviceDescriptor));


    printf("Focus value = %u\n", get_focus_variable(deviceDescriptor));

	set_fps(deviceDescriptor,30);


	size_t buffers_count = start_capturing(deviceDescriptor);


	unsigned int counter;



	int announce_socket=socket(AF_INET,SOCK_DGRAM,0);
	if (announce_socket < 0) {
	  perror("socket");
	  exit(1);
	}

	sockaddr_in announce_address;
	memset(&announce_address,0,sizeof(announce_address));
	announce_address.sin_family=AF_INET;
	announce_address.sin_addr.s_addr=inet_addr(CAMERA_ANNOUNCE_GROUP);
	announce_address.sin_port=htons(CAMERA_ANNOUNCE_PORT);

	while (running != 0) {
		fd_set fds;
		int r;

		FD_ZERO(&fds);
		FD_SET(deviceDescriptor, &fds);

		r = select(deviceDescriptor + 1, &fds, NULL, NULL, NULL);

//		std::cout << "Frame counter: " << counter << std::endl;

		if (r > 0) {
			struct v4l2_buffer buf;

			memset(&buf, 0, sizeof(buf));

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;

			if (-1 == xioctl(deviceDescriptor, VIDIOC_DQBUF, &buf)) {
				switch (errno) {
				case EAGAIN:
					continue;

				case EIO:
					/* Could ignore EIO, see spec. */

					/* fall through */

				default:
					perror("VIDIOC_DQBUF");
					exit(1);
				}
			}
			if ((buf.flags | V4L2_BUF_FLAG_ERROR) != 0) {
				std::cerr << "Frame buffer error" << std::endl;
			}

			printf("Index = %u, seconds = %ld us = %ld\n", buf.index,buf.timestamp.tv_sec,buf.timestamp.tv_usec);
		//	printf("Real time: seconds = %ld, us = %ld\n", tp.tv_sec,tp.tv_nsec/1000);

//			timespec tp;
//			clock_gettime(CLOCK_MONOTONIC,&tp);

/*
			void* source_mapping = mmap (NULL, buf.bytesused,
						 PROT_READ,
						 MAP_SHARED,
						 deviceDescriptor, buf.m.offset);
			if (source_mapping == MAP_FAILED) {
				printf("mmap failed");
				return 1;
			}
*/
			int ret;
			assert(ptrToSequenceMap.count(buf.m.userptr) != 0);
			size_t sequence_number = ptrToSequenceMap[buf.m.userptr];
			ptrToSequenceMap.erase(buf.m.userptr);

			std::cout << "userptr: " << buf.m.userptr << "length:" << buf.length << std::endl;


//			{
//				std::string queued_buffer_name = get_name_of_queued_buffer(buf.index);
//				std::string buffer_name = get_name_of_buffer(buf.timestamp);
//				chmod(("/dev/shm/" + queued_buffer_name).c_str(),S_IRUSR | S_IRGRP | S_IROTH);
//				int ret = rename(("/dev/shm/" + queued_buffer_name).c_str(),("/dev/shm/" + dequeued_buffer_name).c_str());
//				if (ret == -1) {
//					throw std::runtime_error("Failed to rename file " + queued_buffer_name + " to " + dequeued_buffer_name);
//				}
//
//			}

			queueNextFrameBuffer(deviceDescriptor, buf.index, sequence_number, CAMERA_FRAME_WIDTH*CAMERA_FRAME_HEIGHT*2);

//TODO Investigate why the video streaming fails if the unmap call below is placed before the queueNextFrameBuffer call above.
//Probably this is because in that case the mmap call returns the same virtual address as the munmap call had just used for the deallocation
			ret = munmap(reinterpret_cast<void*>(buf.m.userptr),buf.length);
			if (ret == -1) {
				perror("munmap");
			}



			std::cout << buf.sequence << std::endl;
			BufferReference readyBuffer;
			readyBuffer.index = buf.index;
			readyBuffer.offset = 0;
			readyBuffer.size = buf.bytesused;
		//DONE change back to frame timestamp
//			readyBuffer.timestamp_seconds = tp.tv_sec;
//			readyBuffer.timestamp_microseconds = tp.tv_nsec/1000;
			readyBuffer.timestamp_seconds = buf.timestamp.tv_sec;
			readyBuffer.timestamp_microseconds = buf.timestamp.tv_usec;

			readyBuffer.width = CAMERA_FRAME_WIDTH;
			readyBuffer.height = CAMERA_FRAME_HEIGHT;
			readyBuffer.sequence = sequence_number;

			std::array<char,1024> ipc_buffer;
			asn_enc_rval_t encode_result = der_encode_to_buffer(&asn_DEF_BufferReference, &readyBuffer,ipc_buffer.data(),ipc_buffer.size());


			ret = sendto(announce_socket,ipc_buffer.data(),encode_result.encoded,0,(struct sockaddr *) &announce_address,sizeof(announce_address));
			if (ret < 0) {
			   perror("sendto");
			   exit(1);
			}
			count++;

			counter++;


		}
	}

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(deviceDescriptor, VIDIOC_STREAMOFF, &type))
		perror("VIDIOC_STREAMOFF");

	if (-1 == close(deviceDescriptor))
		perror("close");

	close(announce_socket);

	std::cout << "count:" << count << std::endl;
}

int main(int, char**) {
	try {
		camera_server();
	} catch (std::exception& exception) {
		std::cerr << "Exception in the main thread: " << exception.what() << std::endl;
		void *array[10];
		size_t size;

		// get void*'s for all entries on the stack
		size = backtrace(array, 10);

		// print out all the frames to stderr
		backtrace_symbols_fd(array, size, 2);

		return 1;
	} catch (...) {
		std::cerr << "Nonstandard exception in the main thread" << std::endl;
		return 1;
	}

	return 0;
}
