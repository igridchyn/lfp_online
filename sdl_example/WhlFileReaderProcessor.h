/*
 * WhlFileReaderProcessor.h
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#ifndef WHLFILEREADERPROCESSOR_H_
#define WHLFILEREADERPROCESSOR_H_

#include "LFPProcessor.h"

class WhlFileReaderProcessor: public virtual LFPProcessor {
	const std::string whl_path_;
	std::ifstream whl_stream_;

	float sampling_rate_ = .0f;
	int last_pos_pkg_id_ = 0;

	const float SUB_X;
	const float SUB_Y;

public:
	WhlFileReaderProcessor(LFPBuffer *buffer);
	WhlFileReaderProcessor(LFPBuffer *buffer, const std::string& whl_path, const unsigned int& sampling_rate,
			const float sub_x, const float sub_y);
	virtual ~WhlFileReaderProcessor();

	//LFPProcessor
	virtual void process();
	virtual inline std::string name() { return "WHL File Reader"; }
};

#endif /* WHLFILEREADERPROCESSOR_H_ */
