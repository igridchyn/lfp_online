/*
 * SwReaderProcessor.h
 *
 *  Created on: Jun 30, 2014
 *      Author: igor
 */

#ifndef SWREADERPROCESSOR_H_
#define SWREADERPROCESSOR_H_

#include "LFPProcessor.h"

enum SWR_Format{
	SWR_FORMAT_START_PEAK_END,
	SWR_FORMAT_SYNCHRONY_START
};

class SwReaderProcessor: public LFPProcessor {
	std::vector<std::vector<unsigned int>> swrs_;

	// current pkg_id is within or before this SW
	unsigned int current_sw_ = 0;
	SWR_Format swr_format_ = SWR_FORMAT_START_PEAK_END;

public:
	SwReaderProcessor(LFPBuffer *buf);
	SwReaderProcessor(LFPBuffer *buf, std::string path);
	virtual ~SwReaderProcessor();

	// LFProcessor
	virtual void process();
	virtual inline std::string name() { return "SW Reader"; }
};

#endif /* SWREADERPROCESSOR_H_ */
