/*
 * FetFileReaderProcessor.h
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#ifndef FETFILEREADERPROCESSOR_H_
#define FETFILEREADERPROCESSOR_H_

#include "LFPProcessor.h"

#include <fstream>
#include <iostream>
#include <vector>

class FetFileReaderProcessor: public LFPProcessor {
	const std::string fet_path_base_;
	std::vector< std::ifstream * > fet_streams_;

	const unsigned int SAMPLING_RATE = 1000;
	unsigned int last_pkg_id_;

	unsigned int num_files_with_spikes_;

	std::vector<bool> file_over_;
	std::vector<Spike *> last_spikies_;

	Spike* readSpikeFromFile(const unsigned int tetr);

public:
	FetFileReaderProcessor(LFPBuffer *buffer, const std::string fet_path_base, int *tetrode_numbers);
	virtual ~FetFileReaderProcessor();

	// LFPProcessor
	virtual void process();
};

#endif /* FETFILEREADERPROCESSOR_H_ */
