/*
 * ParallelPipelineProcessor.h
 *
 *  Created on: May 20, 2015
 *      Author: igor
 */

#ifndef PARALLELPIPELINEPROCESSOR_H_
#define PARALLELPIPELINEPROCESSOR_H_

#include "LFPProcessor.h"
#include <condition_variable>

class ParallelPipelineProcessor: public LFPProcessor {
	unsigned int NGROUP = 0;

	std::vector< LFPProcessor *> processors_;

	// to notify tetrode threads that data is available
	std::condition_variable cv_data_added_[100];
	std::condition_variable cv_job_over_;

	std::vector<bool> data_added_;
	unsigned int threads_finished_;

	std::mutex mtx_data_add_[100];
	std::mutex mtx_data_processed_;

	// worker threads
	std::vector< std::thread * > worker_threads_;

public:
	virtual void process();

	void process_thread(const int group);

	ParallelPipelineProcessor(LFPBuffer *buf);
	virtual ~ParallelPipelineProcessor();
};

#endif /* PARALLELPIPELINEPROCESSOR_H_ */
