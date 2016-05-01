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

enum Whlformat{
	// x y
	WHL_FORMAT_SHORT,
	// big_x big_y small_x small_y valid
	WHL_FORMAT_LONG
};

class FetFileReaderProcessor: public virtual LFPProcessor {
	std::string fet_path_base_;
	std::vector< std::ifstream * > fet_streams_;

	const unsigned int SAMPLING_RATE = 1000;
	unsigned int last_pkg_id_ = 0;

	const unsigned int WINDOW_SIZE;

	std::vector<bool> file_over_;
	std::vector<Spike *> last_spikies_;

	const bool read_spk_;
	std::vector< std::ifstream * > spk_streams_;

	// pos
	const bool read_whl_;
	std::ifstream *whl_file_ = nullptr;

	bool binary_ = false;

	unsigned int report_rate_;

	unsigned int num_files_with_spikes_ = 0;

	unsigned int last_reported_ = 0;

	Spike* readSpikeFromFile(const unsigned int tetr);

	int current_file_ = -1;
	int shift_ = 0;
	std::vector<unsigned int> shifts_;

	double FET_SCALING;

	const unsigned int pos_sampling_rate_;

	unsigned int last_pos_pkg_id_ = 0;

	bool exit_on_over_ = false;

	bool binary_spk_ = false;

	Whlformat whl_format_;

	void openNextFile();

public:
	FetFileReaderProcessor(LFPBuffer *buffer);
	virtual ~FetFileReaderProcessor();

	// LFPProcessor
	virtual void process();
	virtual std::string name() { return "Fet File Reader"; }
};

#endif /* FETFILEREADERPROCESSOR_H_ */
