/*
 * FetFileWriterProcessor.h
 *
 *  Created on: Oct 18, 2014
 *      Author: igor
 */

#ifndef FETFILEWRITERPROCESSOR_H_
#define FETFILEWRITERPROCESSOR_H_

#include "LFPProcessor.h"

class FetFileWriterProcessor : public virtual LFPProcessor {
	// per tetrode
	std::vector<std::ofstream*> fet_files_;
	std::vector< std::ofstream *> spk_files_;

	std::ofstream *whl_file_;
	bool whl_start_written_ = false;

	bool write_spk_;
	bool binary_ = false;

public:
	FetFileWriterProcessor(LFPBuffer *buf);
	virtual ~FetFileWriterProcessor();
	virtual void process();
	virtual inline std::string name() { return "Fet File Writer"; }
};

#endif /* FETFILEWRITERPROCESSOR_H_ */
