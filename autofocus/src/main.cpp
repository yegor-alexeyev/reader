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


#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <errno.h>
#include <vector>
#include <cstring>
#include <cassert>
#include "v4l2.hpp"

#include <array>

#include "libv4lconvert.h"

#include "BufferReference.h"

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

int main() {
    int deviceDescriptor = open ("/dev/video0", O_RDWR /* required */ | O_NONBLOCK, 0);
    if (deviceDescriptor == -1) {
    	std::cout << "Unable to open device";
    	return 1;
    }
//    set_focus_variable(100,deviceDescriptor);


#ifdef USE_OPENCV
    cv::namedWindow("input",1);
#endif


//	std::vector<uint8_t> iminput(width*height*3);

    int bpp = 1;

    bool isAutofocusAvailable = isLogitechAutofocusModeSupported(deviceDescriptor);




    if (isAutofocusAvailable) {
    }


    int mqdes = mq_open("/video0-events2", O_RDONLY | O_CREAT,S_IRWXU | S_IRWXG | S_IRWXO,NULL);
	if (-1 == mqdes) {
		printf("mq_open failed");
		return 1;
	}
	int released_frames_mq = mq_open("/video0-released-frames",
			O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, NULL);
	if (-1 == released_frames_mq) {
		printf("mq_open failed");
		return 1;
	}


	std::vector<FrameBuffer> buffers;

	mq_attr attr;
	if (-1 == mq_getattr(mqdes,&attr)) {
		printf("mq_getattr failed");
		return 1;
	}

	if (-1 == mq_getattr(released_frames_mq,&attr)) {
		printf("mq_getattr failed");
		return 1;
	}
	unsigned priority;
	int count = 0;

	std::array<char,1024> mq_buffer;
	while (1) {
		BufferReference readyBuffer;
		memset(&readyBuffer,0,sizeof(readyBuffer));

			std::vector<char> data(attr.mq_msgsize+1);
			int res = mq_receive(mqdes,data.data(),data.size(),&priority);
		//	continue;
			if (res == -1) {
				std::cout << "Message queue has been broken" << std::endl;
				break;
			}
	    	timespec tp;
	    	clock_gettime(CLOCK_MONOTONIC,&tp);

			void* input_pointer = &readyBuffer;

			ber_decode(0,&asn_DEF_BufferReference,&input_pointer,data.data(),res);

        uint32_t index = readyBuffer.index;
		#ifdef USE_OPENCV
			cv::Mat output(readyBuffer.height,readyBuffer.width,CV_8UC3);
		#endif



//    	printf("Index = %u, seconds = %ld ms = %ld\n", index,readyBuffer.timestamp_seconds,readyBuffer.timestamp_microseconds);
    	printf("Real time diff: Index = %u, seconds = %ld, us = %ld\n", index, tp.tv_sec - readyBuffer.timestamp_seconds,(1000 + tp.tv_nsec/1000 - readyBuffer.timestamp_microseconds)%1000);

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
		cv::cvtColor(input,output,CV_YUV2BGR_YUY2);
#endif

		asn_enc_rval_t encode_result = der_encode_to_buffer(&asn_DEF_BufferReference, &readyBuffer,mq_buffer.data(),mq_buffer.size());

		mq_send(released_frames_mq,mq_buffer.data(),encode_result.encoded,0);

#ifdef USE_OPENCV
		cv::imshow("input", output);
#endif

        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.index = index;


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

	return 0;
}
