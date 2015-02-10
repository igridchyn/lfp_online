/*
 * BinFileReaderProcessor.h
 *
 *  Created on: Feb 10, 2015
 *      Author: igor
 */

#ifndef BINFILEREADERPROCESSOR_H_
#define BINFILEREADERPROCESSOR_H_

#include "LFPProcessor.h"

class BinFileReaderProcessor: public LFPProcessor {
	std::string file_path_;
	const unsigned int chunk_size_;

	FILE *bin_file_;

	const unsigned int nblock_;

	unsigned char *block_;

	bool end_bin_file_reported_ = false;

public:
	BinFileReaderProcessor(LFPBuffer *buf);

	// LFPProcessor
	virtual void process();
	virtual ~BinFileReaderProcessor();
};

#endif /* BINFILEREADERPROCESSOR_H_ */
