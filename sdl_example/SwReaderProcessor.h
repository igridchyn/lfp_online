/*
 * SwReaderProcessor.h
 *
 *  Created on: Jun 30, 2014
 *      Author: igor
 */

#ifndef SWREADERPROCESSOR_H_
#define SWREADERPROCESSOR_H_

#include "LFPProcessor.h"

class SwReaderProcessor: public LFPProcessor {
	std::vector<std::vector<unsigned int>> swrs_;

	// current pkg_id is within or before this SW
	int current_sw_ = 0;

public:
	SwReaderProcessor(LFPBuffer *buf);
	SwReaderProcessor(LFPBuffer *buf, std::string path);
	virtual ~SwReaderProcessor();

	// LFProcessor
	virtual void process();
	virtual inline std::string name() { return "SW Reader"; }
};

#endif /* SWREADERPROCESSOR_H_ */
