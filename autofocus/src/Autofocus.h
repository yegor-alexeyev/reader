/*
 * Autofocus.h
 *
 *  Created on: Jan 22, 2013
 *      Author: yegor
 */

#ifndef AUTOFOCUS_H_
#define AUTOFOCUS_H_

#include "yuy2.h"

#include <time.h>
#include "FocusAnalyser.h"

class Autofocus {
	timespec focus_change_time;
	FocusAnalyser detector;
    int iteration_number = 1;
    size_t indexOfMaximumFromZero;
    int deviceDescriptor;
    bool focusComplete = false;

	void setFocus(uint8_t focusValue);

public:
	Autofocus(int deviceDescriptor);
	void submitFrame(const timeval& timestamp, const yuy2::c_view_t& frame);
	bool isFocusComplete() {
		return focusComplete;
	}
};

#endif /* AUTOFOCUS_H_ */
