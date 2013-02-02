#ifndef V4L2_HPP_
#define V4L2_HPP_

#include <libv4l2.h>

#include <sys/ioctl.h>
#include <errno.h>
#include <stdexcept>

#include <linux/videodev2.h>


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

inline bool isLogitechAutofocusModeSupported(int deviceDescriptor) {
	v4l2_queryctrl queryctrl;
	memset (&(queryctrl), 0, sizeof (queryctrl));

	queryctrl.id = V4L2_CID_FOCUS_LOGITECH;
	return xioctl(deviceDescriptor, VIDIOC_QUERYCTRL, &queryctrl) != -1;

}

inline bool isControlSupported(int deviceDescriptor, int cid) {
	v4l2_queryctrl queryctrl;
	memset (&(queryctrl), 0, sizeof (queryctrl));

	queryctrl.id = cid;
	return xioctl(deviceDescriptor, VIDIOC_QUERYCTRL, &queryctrl) != -1;

}


inline bool isStreamingIOSupported(int deviceDescriptor) {
	v4l2_capability queryctrl;
	memset (&(queryctrl), 0, sizeof (queryctrl));

	bool is_call_successfull = xioctl(deviceDescriptor, VIDIOC_QUERYCAP, &queryctrl) != -1;
	return is_call_successfull && (queryctrl.capabilities & V4L2_CAP_STREAMING) != 0;
}



inline uint8_t get_gain( int deviceDescriptor) {
	v4l2_control control {V4L2_CID_GAIN,0};

	int ret = xioctl(deviceDescriptor, VIDIOC_G_CTRL, &control) != -1;
	assert(ret != -1);
	assert(control.value >= 0 && control.value <= 0xFF);

	return control.value;
}



inline void set_gain(int deviceDescriptor,uint8_t value) {
	v4l2_control control {V4L2_CID_GAIN,value};

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}






inline uint8_t get_focus_variable( int deviceDescriptor) {
	v4l2_control control {V4L2_CID_FOCUS_LOGITECH,0};

	int ret = xioctl(deviceDescriptor, VIDIOC_G_CTRL, &control) != -1;
	assert(ret != -1);
	assert(control.value >= 0 && control.value <= 0xFF);

//	printf("get focus value: %d\n", control.value);

	return control.value;
}


inline void set_fps( int deviceDescriptor, int fps) {

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



inline void set_focus_variable(int deviceDescriptor,uint8_t value) {
	v4l2_control control {V4L2_CID_FOCUS_LOGITECH,value};

//	printf("set focus value: %d\n", control.value);

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}

inline void set_auto_white_balance(int deviceDescriptor, bool value) {
	v4l2_control control {V4L2_CID_AUTO_WHITE_BALANCE,value ? 1 : 0};
	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}


inline bool is_auto_white_balance_set(int deviceDescriptor) {
	v4l2_control control {V4L2_CID_AUTO_WHITE_BALANCE,0};

	int ret = xioctl(deviceDescriptor, VIDIOC_G_CTRL, &control) != -1;
	assert(ret != -1);
	return control.value != 0;
}

inline void set_absolute_exposure(uint16_t value, int deviceDescriptor) {
	v4l2_control control {V4L2_CID_EXPOSURE_ABSOLUTE,value};

	printf("set absolute exposure: %d\n", control.value);

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}



inline bool is_exposure_auto_priority( int deviceDescriptor) {
	v4l2_control control {V4L2_CID_EXPOSURE_AUTO_PRIORITY,0};

	int ret = xioctl(deviceDescriptor, VIDIOC_G_CTRL, &control) != -1;
	assert(ret != -1);
	return control.value != 0;
}

inline void set_exposure_auto_priority( int deviceDescriptor, bool value) {
	v4l2_control control {V4L2_CID_EXPOSURE_AUTO_PRIORITY,value ? 1 : 0};

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}


inline bool is_manual_exposure( int deviceDescriptor) {
	v4l2_control control {V4L2_CID_EXPOSURE_ABSOLUTE,0};

	int ret = xioctl(deviceDescriptor, VIDIOC_G_CTRL, &control) != -1;
	assert(ret != -1);
	return control.value == V4L2_EXPOSURE_MANUAL;
}


inline void set_manual_exposure( int deviceDescriptor, bool value) {
	v4l2_control control {V4L2_CID_EXPOSURE_ABSOLUTE,value ? V4L2_EXPOSURE_MANUAL : V4L2_EXPOSURE_APERTURE_PRIORITY};

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}


inline void disable_output_processing( int deviceDescriptor) {
	v4l2_control control {V4L2_CID_DISABLE_PROCESSING_LOGITECH ,0};

	int ret = xioctl(deviceDescriptor, VIDIOC_S_CTRL, &control) != -1;
	assert(ret != -1);
}

inline void setCameraOutputFormat(int deviceDescriptor, size_t frameWidth, size_t frameHeight, __u32 pixelFormat) {
	v4l2_format format;
	memset(&format, 0, sizeof(v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret = ioctl(deviceDescriptor, VIDIOC_G_FMT, &format);
	if (ret == -1) {
		throw std::runtime_error("Capture stream type is not supported");
	}

	memset(&format, 0, sizeof(v4l2_format));
	format.fmt.pix.width = frameWidth;
	format.fmt.pix.height = frameHeight;
//	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	format.fmt.pix.pixelformat = pixelFormat;

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(deviceDescriptor, VIDIOC_S_FMT, &format);
	if (ret == -1) {
		throw std::runtime_error("Format is not supported");
	}
}

inline void queue_frame_buffer(int device_descriptor, __u32 buffer_index, void* userptr, size_t buffer_length) {
	v4l2_buffer buffer;
	memset(&buffer, 0, sizeof(buffer));
	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_USERPTR;
	buffer.index = buffer_index;
	buffer.m.userptr = reinterpret_cast<unsigned long>(userptr);
	buffer.length = buffer_length;

	if (-1 == xioctl(device_descriptor, VIDIOC_QBUF, &buffer)) {
		perror("VIDIOC_QBUF");
		throw std::runtime_error("VIDIOC_QBUF");
	}
}

#endif /* V4L2_HPP_ */

