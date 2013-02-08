#ifndef IPC_HPP_
#define IPC_HPP_
#include <signal.h>

#define CAMERA_ANNOUNCE_PORT 2013
#define CAMERA_ANNOUNCE_GROUP "226.0.0.175"

inline std::string get_name_of_buffer(uint32_t sequence_number) {
	return "video0-" + std::to_string(sequence_number);
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


#endif
