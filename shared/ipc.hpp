#ifndef IPC_HPP_
#define IPC_HPP_

#define CAMERA_ANNOUNCE_PORT 2013
#define CAMERA_ANNOUNCE_GROUP "226.0.0.175"

inline std::string get_name_of_dequeued_buffer(timeval capture_time) {
	return "video0-ready-" + std::to_string(capture_time.tv_sec) + "." + std::to_string(capture_time.tv_usec);
}

#endif
