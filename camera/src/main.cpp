#include <stdio.h>
#include <sys/stat.h>

#include <signal.h>
#include <mqueue.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <libv4l2.h>

#include <linux/videodev2.h>

#include <vector>
#include <assert.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "v4l2.hpp"
#include "ipc.hpp"

#include <array>

#include "BufferReference.h"

struct frame_buffer {
	void* start_address;
	size_t length;
	int index;
};

const uint32_t CAMERA_FRAME_WIDTH = 640;
const uint32_t CAMERA_FRAME_HEIGHT = 480;

std::vector<frame_buffer> map_buffers(int deviceDescriptor) {
	struct v4l2_requestbuffers reqbuf;

	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = 20;

	if (-1 == ioctl(deviceDescriptor, VIDIOC_REQBUFS, &reqbuf)) {
		if (errno == EINVAL)
			printf("Video capturing or mmap-streaming is not supported\n");
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
		v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (-1 == ioctl(deviceDescriptor, VIDIOC_QUERYBUF, &buffer)) {
			perror("VIDIOC_QUERYBUF");
			exit(EXIT_FAILURE);
		}

		mapped_buffer.index = buffer.index;
		mapped_buffer.length = buffer.length;
		mapped_buffer.start_address = mmap(NULL, buffer.length, PROT_READ,
				MAP_SHARED, deviceDescriptor, buffer.m.offset);

		if (MAP_FAILED == mapped_buffer.start_address) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}
	}

	return buffers;

}

void start_capturing(int deviceDescriptor,
		const std::vector<frame_buffer>& mapped_buffers) {
	enum v4l2_buf_type type;
	for (const frame_buffer& mapped_buffer : mapped_buffers) {
		v4l2_buffer buf;

		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = mapped_buffer.index;

		if (-1 == xioctl(deviceDescriptor, VIDIOC_QBUF, &buf))
			perror("VIDIOC_QBUF");
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(deviceDescriptor, VIDIOC_STREAMON, &type))
		perror("VIDIOC_STREAMON");
}

void unmap_buffers(std::vector<frame_buffer> buffers) {
	for (frame_buffer buffer : buffers) {
		munmap(buffer.start_address, buffer.length);
	}
}

int read_frame(int deviceDescriptor, int mqdes,
		const std::vector<frame_buffer>& buffers) {

}

sig_atomic_t volatile running = 1;

void termination_handler(int signal) {
	running = 0;
}

int main(int, char**) {

	struct sigaction termination;
	memset(&termination, 0, sizeof(struct sigaction));
	termination.sa_handler = &termination_handler;
	sigemptyset(&termination.sa_mask);
	termination.sa_flags = 0;
	sigaction(SIGTERM, &termination, NULL);

	int deviceDescriptor = v4l2_open("/dev/video0",
			O_RDWR /* required */| O_NONBLOCK, 0);
	if (deviceDescriptor == -1) {
		printf("Unable to open device");
		return 1;
	}

	disable_output_processing(deviceDescriptor);

	v4l2_format format;
	memset(&format, 0, sizeof(v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret = ioctl(deviceDescriptor, VIDIOC_G_FMT, &format);
	if (ret == -1) {
		printf("Capture stream type is not supported");
		return 1;
	}
//	set_absolute_exposure(9000,deviceDescriptor);

	memset(&format, 0, sizeof(v4l2_format));
	format.fmt.pix.width = CAMERA_FRAME_WIDTH;
	format.fmt.pix.height = CAMERA_FRAME_HEIGHT;
//	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(deviceDescriptor, VIDIOC_S_FMT, &format);
	if (ret == -1) {
		printf("Format is not supported");
		return 1;
	}

	set_fps(deviceDescriptor,30);


	std::vector<frame_buffer> buffers = map_buffers(deviceDescriptor);
	start_capturing(deviceDescriptor, buffers);

	int released_frames_mq = mq_open("/video0-released-frames",
			O_RDONLY | O_CREAT | O_NONBLOCK, S_IRWXU | S_IRWXG | S_IRWXO, NULL);

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
			buf.memory = V4L2_MEMORY_MMAP;

			if (-1 == xioctl(deviceDescriptor, VIDIOC_DQBUF, &buf)) {
				switch (errno) {
				case EAGAIN:
					continue;

				case EIO:
					/* Could ignore EIO, see spec. */

					/* fall through */

				default:
					perror("VIDIOC_DQBUF");
				}
			}


			printf("Index = %u, seconds = %ld us = %ld\n", buf.index,buf.timestamp.tv_sec,buf.timestamp.tv_usec);
		//	printf("Real time: seconds = %ld, us = %ld\n", tp.tv_sec,tp.tv_nsec/1000);

			timespec tp;
			clock_gettime(CLOCK_MONOTONIC,&tp);


			BufferReference readyBuffer;
			readyBuffer.index = buf.index;
			readyBuffer.offset = buf.m.offset;
			readyBuffer.size = buf.bytesused;
		//TODO change back to frame timestamp
			readyBuffer.timestamp_seconds = tp.tv_sec;
			readyBuffer.timestamp_microseconds = tp.tv_nsec/1000;
		//	readyBuffer.timestamp_seconds = buf.timestamp.tv_sec;
		//	readyBuffer.timestamp_microseconds = buf.timestamp.tv_usec;

			readyBuffer.width = CAMERA_FRAME_WIDTH;
			readyBuffer.height = CAMERA_FRAME_HEIGHT;

			std::array<char,1024> ipc_buffer;
			asn_enc_rval_t encode_result = der_encode_to_buffer(&asn_DEF_BufferReference, &readyBuffer,ipc_buffer.data(),ipc_buffer.size());


			int ret = sendto(announce_socket,ipc_buffer.data(),encode_result.encoded,0,(struct sockaddr *) &announce_address,sizeof(announce_address));
			if (ret < 0) {
			   perror("sendto");
			   exit(1);
			}

			counter++;
		}

		std::array<char,16384> message_buffer;
		while (true) {
			int result = mq_receive(released_frames_mq, message_buffer.data(),message_buffer.size(),NULL);
			if (result == -1) {
				break;
			}
			BufferReference buffer_reference;
			memset(&buffer_reference,0,sizeof(BufferReference));
			void* input_pointer = &buffer_reference;
			ber_decode(0,&asn_DEF_BufferReference,&input_pointer,message_buffer.data(),result);
			struct v4l2_buffer buffer;
			memset(&buffer, 0, sizeof(buffer));
			buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buffer.index = buffer_reference.index;
			buffer.memory = V4L2_MEMORY_MMAP;

			if (-1 == xioctl(deviceDescriptor, VIDIOC_QBUF, &buffer))
				perror("VIDIOC_QBUF");
		}
	}

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(deviceDescriptor, VIDIOC_STREAMOFF, &type))
		perror("VIDIOC_STREAMOFF");

	unmap_buffers(buffers);

	if (-1 == close(deviceDescriptor))
		perror("close");

	close(announce_socket);

	return 0;
}

