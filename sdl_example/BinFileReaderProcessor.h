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
	std::vector<std::string> files_list_;
	const unsigned int chunk_size_;

	FILE *bin_file_;

	const unsigned int nblock_;

	unsigned char *block_;

	bool end_bin_file_reported_ = false;

	unsigned int current_file_ = 0;

	// for fake environment separation assuming that subsequent files represent different environments
	float x_shift_upon_file_change_ = .0;
	float y_shift_upon_file_change_ = .0;

	unsigned int totalAxonaPackages(std::vector<std::string> file_list);
	std::string axonaFileDurationFromNSampes(const unsigned int& nsamples);
	std::string axonaFileDuration(std::string file_path);
	std::string axonaFilesDuration(std::vector<std::string> file_list);

public:
	BinFileReaderProcessor(LFPBuffer *buf);

	// LFPProcessor
	virtual void process();
	virtual inline std::string name() { return "Bin File Reader"; }
	virtual ~BinFileReaderProcessor();
};

#endif /* BINFILEREADERPROCESSOR_H_ */
