#ifndef IPC_HPP_
#define IPC_HPP_

#define CAMERA_ANNOUNCE_PORT 2013
#define CAMERA_ANNOUNCE_GROUP "226.0.0.175"

inline std::string get_name_of_buffer(uint32_t sequence_number) {
	return "video0-" + std::to_string(sequence_number);
}

#endif
