/*
 * camera.hpp
 *
 *  Created on: Dec 19, 2012
 *      Author: yegor
 */

#ifndef CAMERA_HPP_
#define CAMERA_HPP_

#define V4L2_CID_BASE_EXTCTR                0x0A046D01
#define V4L2_CID_BASE_LOGITECH              V4L2_CID_BASE_EXTCTR
#define V4L2_CID_FOCUS_LOGITECH             V4L2_CID_BASE_LOGITECH + 3
#define V4L2_CID_DISABLE_PROCESSING_LOGITECH V4L2_CID_BASE_LOGITECH + 0x70


static int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

bool isLogitechAutofocusModeSupported(int deviceDescriptor) {
	v4l2_queryctrl queryctrl;
	memset (&(queryctrl), 0, sizeof (queryctrl));

	queryctrl.id = V4L2_CID_FOCUS_LOGITECH;
	return xioctl(deviceDescriptor, VIDIOC_QUERYCTRL, &queryctrl) != -1;

}

bool isStreamingIOSupported(int deviceDescriptor) {
	v4l2_capability queryctrl;
	memset (&(queryctrl), 0, sizeof (queryctrl));

	bool is_call_successfull = xioctl(deviceDescriptor, VIDIOC_QUERYCAP, &queryctrl) != -1;
	return is_call_successfull && (queryctrl.capabilities & 0x04000000) != 0;
}






uint8_t get_focus_variable( int deviceDescriptor) {
	v4l2_control control {V4L2_CID_FOCUS_LOGITECH,0};

	int ret = xioctl(deviceDescriptor, VIDIOC_G_CTRL, &control) != -1;
	assert(ret != -1);
	assert(control.value >= 0 && control.value <= 0xFF);

	printf("get focus value: %d\n", control.value);

	return control.value;
}


void set_fps( int deviceDescriptor, int fps) {

	v4l2_streamparm streamparm;
	memset(&streamparm,0,sizeof(streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = xioctl(deviceDescriptor, VIDIOC_G_PARM, &streamparm);
	assert(ret != -1);
	assert(streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME);


    streamparm.parm.capture.timeperframe.numerator = 1;
    streamparm.parm.capture.timeperframe.denominator = fps;

    ret = xioctl(deviceDescriptor, VIDIOC_S_PARM, &streamparm);
	assert(ret != -1);

	printf("fps set: %d/%d\n", streamparm.parm.capture.timeperframe.denominator/streamparm.parm.capture.timeperframe.numerator);
}



void set_focus_variable(uint8_t value, int deviceDescriptor) {
	v4l2_control control {V4L2_CID_FOCUS_LOGITECH,value};

	printf("set focus value: %d\n", control.value);

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}

void set_absolute_exposure(uint16_t value, int deviceDescriptor) {
	v4l2_control control {V4L2_CID_EXPOSURE_ABSOLUTE,value};

	printf("set absolute exposure: %d\n", control.value);

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}


void disable_output_processing( int deviceDescriptor) {
	v4l2_control control {V4L2_CID_DISABLE_PROCESSING_LOGITECH ,0};

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}


//
//v4l2_fmtdesc fmtdesc;
//fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//fmtdesc.index = 0;
//while (true) {
//	ret = ioctl(	deviceDescriptor, VIDIOC_ENUM_FMT, &fmtdesc);
//	if (ret == -1) {
//		printf("Done");
//		return 1;
//	}
//	fmtdesc.index++;
//}


#endif /* CAMERA_HPP_ */
