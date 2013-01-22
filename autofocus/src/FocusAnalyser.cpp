/*
 * FocusAnalyser.cpp
 *
 *  Created on: Jan 22, 2013
 *      Author: yegor
 */

#include "FocusAnalyser.h"

void FocusAnalyser::addFocusMeasurementResult(uint32_t focusSum) {
	if (measurementCount >= 5) {
		if ((history[0] == history[1] && history[1] == focusSum)
				|| (history[0] < history[1] && history[1] > focusSum)
				|| (history[0] > history[1] && history[1] < focusSum)) {
			stabiliseCount++;
		} else {
			stabiliseCount = 0;
		}
	}
	history.push_back(focusSum);
	measurementCount++;
	if (maximumValue < focusSum) {
		maximumValueIndex = measurementCount;
		maximumValue = focusSum;
	}
}
