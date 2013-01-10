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


/*
uint32_t calculateDiffSum(cv::Mat frame) {
    uint32_t diffSum = 0;
    size_t frame_width = 32;
    size_t frame_height = 32;

    for (int y = frame.rows/2 - frame_height/2; y>0 && y < frame.rows && y < frame.rows/2 + frame_height/2; y++) {
        for (int x = frame.cols/2 - frame_width/2; x>0 && x < frame.cols && x < frame.cols/2 + frame_width/2; x++) {
         	diffSum += abs(frame.at<uint8_t>(x,y) - frame.at<uint8_t>(x-1,y));
         	diffSum += abs(frame.at<uint8_t>(x,y) - frame.at<uint8_t>(x,y-1));
        }
    }
//    printf("Diff sum = %u\n", diffSum);
    return diffSum;

}
*/


void fillWithLumaFromYUY2(uint8_t yuy2[],size_t width,size_t height,uint8_t luma[]) {
	for (size_t row = 0; row < height; row++) {
		for (size_t column = 0; column < width; column++) {
//		for (size_t column = 0; column < width; column+=2) {
			luma[row*width+column] = yuy2[row*width*2+column*2+1];
//			luma[row*width+column+1] = yuy2[row*width*2+column*2+2];
//			luma[row*width+column] = yuy2[row*width+column];

		}
	}
}


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


	std::vector<uint8_t> iminput(1600*1200*3);
//	int f = open("/home/yegor/data/1.raw",O_RDONLY );
//	int f = open("/home/yegor/black.raw",O_RDONLY );
//	int f = open("/home/yegor/green.raw",O_RDONLY );
//	int f = open("/home/yegor/red.raw",O_RDONLY );
//	if (f == -1) {
//		printf("open failed");
//		return 1;
//	}
//	read(f,iminput.data(),1600*1200*3);
//	close(f);
//
//	std::vector<uint8_t> conv(2600*600*3);
//
//
//	for (size_t row = 0; row < 1200; row+=16) {
//		for (size_t introw = 0; introw < 8; introw++) {
//			for (size_t column = 0; column < 1600; column++) {
//				conv[2600*3*(row/2+introw)+column*3] = iminput[1600*3*(row+introw)+column*3];
//				conv[2600*3*(row/2+introw)+column*3+1] = iminput[1600*3*(row+introw)+column*3+1];
//				conv[2600*3*(row/2+introw)+column*3+2] = iminput[1600*3*(row+introw)+column*3+2];
//			}
//		}
//		for (size_t introw = 0; introw < 8; introw++) {
//			for (size_t column = 0; column < 1000; column++) {
//				conv[2600*3*(row/2+introw)+column*3+1600*3] = iminput[1600*3*(row+8+introw)+column*3];
//				conv[2600*3*(row/2+introw)+column*3+1600*3+1] = iminput[1600*3*(row+8+introw)+column*3+1];
//				conv[2600*3*(row/2+introw)+column*3+1600*3+2] = iminput[1600*3*(row+8+introw)+column*3+2];
//			}
//		}
//	}
//
//	cv::Mat mat(600,2600,CV_8UC3);

//	cv::Mat mat(1200,1600,CV_8UC1);


//		fillWithLumaFromYUY2((uint8_t*)buffers[index], width,height, mat.data);
//	cv::imshow("input", mat);
//
//
//	uint32_t offset_val = 0x80000000;
//	std::multiset<uint8_t> sa,sb,sc;
//	std::multiset<uint32_t> r;
//	while (offset_val !=0) {
//    	for (size_t row = 0; row < mat.rows; row++) {
//    		for (size_t column = 0; column < mat.cols; column++) {
////    			bool bit = (conv[row*mat.cols*3+column*3+0] & offset_val) != offset_val;
//
//
//    			uint8_t a = conv[row*mat.cols*3+column*3+0];
////    			a = (a & 0x7E) | (~a & 0x81);
//    			uint8_t b = conv[row*mat.cols*3+column*3+1];
////    			b = (b & 0x7E) | (~b & 0x81);
//    			uint8_t c = conv[row*mat.cols*3+column*3+2];
////    			c = (c & 0x7E) | (~c & 0x81);
//    			uint32_t s = a*0x10000 + b*0x100 + c;
//
//
//    			//    			int i = 0;
////    			while (s != 0) {
////    				if (s%2 == 1) {
////    					i++;
////    				}
////    				s = s >> 1;
////
////    			}
//    			bool bit = s & offset_val;
////    			mat.data[row*mat.cols*3+column*3] = bit ? 255 : 0;
////    			if (a !=0x7e && b!=0x7e && c != 0x7e && a !=0x81 && b!= 0x81 && c!=0x81) {
////        			if (a !=0xFF && b!=0xFF && c != 0xFF && a !=0x0 && b!= 0x0 && c!=0x0) {
//     			mat.data[row*mat.cols*3+column*3] = a;
//    			mat.data[row*mat.cols*3+column*3+1] =  b;
//    			mat.data[row*mat.cols*3+column*3+2] =  c;
////    			} else {
////         			mat.data[row*mat.cols*3+column*3] = s & offset_val? 255:0;
////        			mat.data[row*mat.cols*3+column*3+1] = s& offset_val? 255:0;
////        			mat.data[row*mat.cols*3+column*3+2] = s& offset_val ? 255:0;
////    			}
//
////    			sa.insert(a);
////    			sb.insert(b);
////    			sc.insert(c);
////    			r.insert(s);
//    		}
//    	}
//		cv::imshow("input", mat);
//		offset_val = offset_val << 1;
//
////	for (int i=0;i<256;i++) {
////    	printf("i: %d\n",i);
////    	printf("a: %d\n",sa.count(i));
////    	printf("b: %d\n",sb.count(i));
////    	printf("c: %d\n",sc.count(i));
////
////    }
////	for (uint32_t a: r) {
////    	printf("n,count: %d, %d\n",a,r.count(a));
////	}
//
//	cv::waitKey(0);
//	}
//	return 0;
//

    //    int length = 640*480*2;

    int width = 1600;
    int height = 1200;
    int bpp = 1;

    bool isAutofocusAvailable = isLogitechAutofocusModeSupported(deviceDescriptor);




    if (isAutofocusAvailable) {
    }


    //        Mat frame;

     //       capture >> frame; // get first frame for size
    //        Mat edges;
    //        cvtColor(frame, edges, CV_BGR2GRAY);
    //        uint32_t topDiffSum = calculateDiffSum(frame);

    //        Mat output(frame.size(),CV_32SC1);
    	//        namedWindow("result",1);


//    int focusChangeDelta = val == 255 ? -1 : 1;
//    if (val < 128) {
//    set_focus_variable(128,deviceDescriptor);
//    }




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


	std::vector<void*> buffers;

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


//	v4lconvert_data* convert_data = v4lconvert_create(deviceDescriptor);
//	v4l2_format target_format;


//	memset(&target_format,0,sizeof(v4l2_format));
//	target_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//	int ret = ioctl(	deviceDescriptor, VIDIOC_G_FMT, &target_format);
//	if (ret == -1) {
//		printf("Capture stream type is not supported");
//		return 1;
//	}
//
//	v4l2_format  source_format = target_format;

//	source_format.fmt.pix.pixelformat = V4L2_PIX_FMT_JL2005BCD;
//	target_format.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;

	size_t buffer_size = 1600*1200*2;
//	std::vector<char> imoutput(buffer_size);
	std::array<char,1024> mq_buffer;
#ifdef USE_OPENCV
	cv::Mat output(height,width,CV_8UC3);
#endif
	while (1) {
//		char* d = (char*)data.data();
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



//    	printf("Index = %u, seconds = %ld ms = %ld\n", index,readyBuffer.timestamp_seconds,readyBuffer.timestamp_microseconds);
    	printf("Real time diff: Index = %u, seconds = %ld, us = %ld\n", index, tp.tv_sec - readyBuffer.timestamp_seconds,(1000 + tp.tv_nsec/1000 - readyBuffer.timestamp_microseconds)%1000);

        if (index >= buffers.size()) {
        	buffers.resize(index+1, NULL);
        }
        if (buffers[index] == NULL) {
        	buffers[index] = mmap (NULL, buffer_size,
        				 PROT_READ,
        				 MAP_SHARED,
        				 deviceDescriptor, readyBuffer.offset);
        	if (buffers[index] == MAP_FAILED) {
        		printf("mmap failed");
        		return 1;
        	}
        }


		int err  = errno;
#ifdef USE_OPENCV
		cv::Mat input(height,width,CV_8UC2,(uint8_t*)buffers[index]);
		cv::cvtColor(input,output,CV_YUV2BGR_YUY2);
#endif

		asn_enc_rval_t encode_result = der_encode_to_buffer(&asn_DEF_BufferReference, &readyBuffer,mq_buffer.data(),mq_buffer.size());

		mq_send(released_frames_mq,mq_buffer.data(),encode_result.encoded,0);

//		cv::Mat mat(height,width,CV_8UC1,output.data());

//		int f = open("/home/yegor/data/0.jpg",O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR |S_IWUSR |S_IRGRP |S_IROTH );
//		if (f == -1) {
//			printf("open failed");
//			return 1;
//		}
//		write(f,buffers[index],buffer_size);
//		close(f);




//#ifdef USE_OPENCV
//		cv::Mat mat(height,width,CV_32S);
//		int ret = v4lconvert_convert(convert_data,&source_format,&target_format,(unsigned char*)buffers[index],buffer_size,mat.data,width*height*3);
//		if (ret == -1) {
//			printf("convert failed\n");
//			continue;
//		}

//		fillWithLumaFromYUY2((uint8_t*)buffers[index], width,height, mat.data);
#ifdef USE_OPENCV
		cv::imshow("input", output);
#endif
//#else
//		int ret = v4lconvert_convert(convert_data,&source_format,&target_format,(unsigned char*)buffers[index],buffer_size,imoutput.data(),width*height*3);
//		if (ret == -1) {
//			printf("convert failed\n");
//			continue;
//		}
//#endif
// 	    set_focus_variable(abs(count%511 - 255),deviceDescriptor);
//	    get_focus_variable(deviceDescriptor);

        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.index = index;

//		if (ret != -1) {
//			std::cout << "frame" << std::endl;
			count+=1;
//		}

#ifdef USE_OPENCV
        if(int key = cv::waitKey(1) >= 0) {
        	break;
        }
#endif
	}
	std::cout << "count:" << count << std::endl;


//	v4lconvert_destroy(convert_data);


	/*

	size_t counter = 0;
    for(;;)
    {
    	counter ++;
    		grab_frame_timer.resume();
//        		output = Scalar(0);
            // get a new frame from camera


    		double fps = capture.get(CV_CAP_PROP_FPS);

            capture >> frame;

        	grab_frame_timer.stop();
//                cvtColor(frame, edges, CV_BGR2GRAY);

            uint32_t diffSum = calculateDiffSum(frame);
            int focusValue = get_focus_variable(deviceDescriptor);
            if (diffSum < topDiffSum) {
            	focusChangeDelta = -focusChangeDelta;
            	topDiffSum = 0;
            } else {
            	topDiffSum = diffSum;
            	focusChangeDelta++;
            	if (focusValue == 255) {
            		focusChangeDelta = -1;
            	}
            	if (focusValue == 0) {
            		focusChangeDelta = 1;
            	}

            }

            while (focusValue + focusChangeDelta < 0 || focusValue + focusChangeDelta > 255) {
            	focusChangeDelta/=2;
            }
            focusValue += focusChangeDelta;
        	process_frame_timer.resume();
          	set_focus_variable(focusValue,deviceDescriptor);

        	process_frame_timer.stop();

//              	printf("Focus variable: %u\n", focusValue);

//                cv::Canny(edges,edges,700,2000,7,true);
//
//                cv::vector<cv::vector<cv::Point> > contours;
//                cv::vector<cv::Vec4i> hierarchy;
//
//                cv::findContours(edges, contours, hierarchy, CV_RETR_LIST,CV_CHAIN_APPROX_NONE);
//                cv::Mat::zeros(edges.size(),edges.type());
//                RNG rng(12345);
//
//                for (size_t i = 0; i < contours.size(); i++) {
//                    Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
//                	cv::drawContours(edges,contours,i,color);
//                }
//
//                edges.assignTo(output,CV_32SC1);
//                cv::watershed(frame,output);
//
//
//                output.convertTo(markers8U, CV_8U, 1, 1);

            //                processFrame(edges.data,edges.cols,edges.rows);
            // show frame on screen

//                for (int y = frame.rows/2 - 128; y>0 && y < frame.rows && y < frame.rows/2 + 128; y+= 255) {
//                    for (int x = frame.cols/2 - 128; x>0 && x < frame.cols && x < frame.cols/2 + 128; x+=2) {
//                    	frame.at<uint8_t[3]>(y,x)[0] = 0;
//                    	frame.at<uint8_t[3]>(y,x)[1] = 0;
//                    	frame.at<uint8_t[3]>(y,x)[2] = 0;
//                    }
//                }
//
//                for (int y = frame.rows/2 - 128; y>0 && y < frame.rows && y < frame.rows/2 + 128; y+=2) {
//                    for (int x = frame.cols/2 - 128; x>0 && x < frame.cols && x < frame.cols/2 + 128; x+=255) {
//                    	frame.at<uint8_t[3]>(y,x)[0] = 0;
//                    	frame.at<uint8_t[3]>(y,x)[1] = 0;
//                    	frame.at<uint8_t[3]>(y,x)[2] = 0;
//                    }
//                }


          	gui_timer.resume();
            imshow("input", frame);
          	gui_timer.stop();
//              imshow("result", markers8U);

          	ui_timer.resume();

            if(int key = waitKey(1) >= 0) {
            	break;
            }
          	ui_timer.stop();
    }
  	total_timer.stop();
*/













	for (void* ptr: buffers) {
		munmap (ptr, buffer_size);
	}

	return 0;
}
