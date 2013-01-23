/*
 * Autofocus.cpp
 *
 *  Created on: Jan 22, 2013
 *      Author: yegor
 */

#include "Autofocus.h"

#include "v4l2.hpp"

uint32_t calculate_focus_sum(const yuy2::c_view_t& view) {
	uint32_t focus_sum = 0;
	for (uint32_t row = view.height()/3 +1; row < view.height()*2/3;row++) {
		for (uint32_t column = view.width()/3 +1; column < view.width()*2/3;column++) {
			focus_sum += abs(boost::gil::get_color(*view.at(column,row),yuy2::luma_t()) - boost::gil::get_color(*view.at(column,row - 1),yuy2::luma_t()));
			focus_sum += abs(boost::gil::get_color(*view.at(column,row),yuy2::luma_t()) - boost::gil::get_color(*view.at(column - 1,row),yuy2::luma_t()));
		}
	}
	return focus_sum;
}

void Autofocus::submitFrame(const timeval& timestamp,
		const yuy2::c_view_t& frame) {
	if (timestamp.tv_sec > focus_change_time.tv_sec
			|| (timestamp.tv_sec == focus_change_time.tv_sec
					&& timestamp.tv_usec > (focus_change_time.tv_nsec/1000 + 500))) {
		if (cr_needed) {
			std::cout << "!" << std::endl;
			cr_needed = false;
		}
	}
	if (!cr_needed) {

		uint32_t focus_sum = calculate_focus_sum(frame);
		detector.addFocusMeasurementResult(focus_sum);

		std::cout << focus_sum << ",";
		if (detector.isFocusStabilised()) {
			if (iteration_number >= 2 && iteration_number % 2 == 0) {
				indexOfMaximumFromZero = detector.getMaximumValueIndex();
			}
			if (iteration_number >= 3 && iteration_number % 2 != 0) {
				size_t indexOfMaximumFromMax = detector.getMaximumValueIndex();
				//				std::cout << "Indexes " << indexOfMaximumFromZero/indexOfMaximumFromMax << std::endl;
				std::cout << "Indexes " << indexOfMaximumFromZero << "-"
						<< indexOfMaximumFromMax << std::endl;

			}
			iteration_number++;
			cr_needed = true;
			detector = FocusAnalyser();

			uint8_t focus = get_focus_variable(deviceDescriptor);
			//			if (focus == 255) {
			//				break;
			//			}
			set_focus_variable(deviceDescriptor, focus == 255 ? 0 : 255);
			clock_gettime(CLOCK_MONOTONIC, &focus_change_time);

			//		    set_focus_variable(focus +1,deviceDescriptor);
		}

	}
}


