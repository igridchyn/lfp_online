/*
 * PackageExtractorProcessor.h
 *
 *  Created on: Feb 6, 2015
 *      Author: igor
 */

#ifndef PACKAGEEXTRACTORPROCESSOR_H_
#define PACKAGEEXTRACTORPROCESSOR_H_

#include "LFPProcessor.h"

class PackageExractorProcessor : public virtual LFPProcessor{
	// Axona package configuration in bytes
	const int HEADER_LEN = 32;
	const int TAIL_LEN = 16;

	const float SCALE;
	unsigned int last_reported_ = 0;
	unsigned int report_rate_;
	bool exit_on_data_over_ = false;
	bool end_reported_ = false;
	// ignore every other position sample, because it repeats the previous one
	bool skip_next_pos_ = false;
	bool read_pos_;

	bool skip_next_pkg_ = false;

	const int CHUNK_SIZE;

	bool mode128_ = false;

	// which channel is at i-th position in the BIN chunk
	int *CH_MAP_INV;
	int *CH_MAP;

	time_t last_check_point_ = 0;

	unsigned int total_chunks_ = 0;

public:
	virtual std::string name();
    virtual void process();
    PackageExractorProcessor(LFPBuffer *buffer);
    ~PackageExractorProcessor() { delete[] CH_MAP; delete[] CH_MAP_INV; }
};


#endif /* PACKAGEEXTRACTORPROCESSOR_H_ */
