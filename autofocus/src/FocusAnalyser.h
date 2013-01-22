/*
 * FocusAnalyser.h
 *
 *  Created on: Jan 22, 2013
 *      Author: yegor
 */

#ifndef FOCUSANALYSER_H_
#define FOCUSANALYSER_H_
#include <boost/circular_buffer.hpp>


class FocusAnalyser {
private:
	boost::circular_buffer<uint32_t> history;
	size_t stabiliseCount;
	size_t measurementCount;
	uint32_t maximumValue;
	size_t maximumValueIndex;
public:
	FocusAnalyser() :
		history(2),
		stabiliseCount(0),
		measurementCount(0),
		maximumValue(0),
		maximumValueIndex(0)
	{

	}
	size_t getMaximumValue() {
		return maximumValue;
	}


	void addFocusMeasurementResult(uint32_t focusSum);

	size_t getMaximumValueIndex() {
		return maximumValueIndex;
	}

	bool isFocusStabilised() {
		return stabiliseCount >= 3;
	}
};
#endif /* FOCUSANALYSER_H_ */
