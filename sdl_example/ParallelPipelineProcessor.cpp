/*
 * ParallelPipelineProcessor.cpp
 *
 *  Created on: May 20, 2015
 *      Author: igor
 */

#include "ParallelPipelineProcessor.h"

ParallelPipelineProcessor::ParallelPipelineProcessor(LFPBuffer *buf)
	: LFPProcessor(buf) {
	NGROUP = 8;

	threads_finished_ = 0;
}

void ParallelPipelineProcessor::process() {
	// DEBUG
//	std::cout << "Set data add to 0 and notify all ... \n";

	// PROFILE
//	clock_t start = clock();

	for (unsigned int g = 0; g < NGROUP; ++g) {
		std::unique_lock<std::mutex> lk(mtx_data_add_[g]);
		data_added_[g] = true;
		cv_data_added_[g].notify_one();
	}

//	cv_data_added_.notify_all();

	std::unique_lock<std::mutex> lk(mtx_data_processed_);
	cv_job_over_.wait(lk, [=]{return threads_finished_ == NGROUP;});
	threads_finished_ = 0;

	// PROFILE
//	time_t time = clock() - start;
//	std::cout << "All jobs over ... pkg id = " << buffer->last_pkg_id << ", time (us) = " << (time * 1000000) / CLOCKS_PER_SEC <<  "\n";
}

void ParallelPipelineProcessor::process_thread(const int group) {
	while(true){
		std::unique_lock<std::mutex> lk(mtx_data_add_[group]);
		cv_data_added_[group].wait(lk, [=]{return data_added_[group];});

		for (std::vector<LFPProcessor*>::const_iterator piter = processors_.begin(); piter != processors_.end(); ++piter) {
			(*piter)->process();
		}

		{
			std::unique_lock<std::mutex> lkdp(mtx_data_processed_);
			threads_finished_ ++;
			lkdp.unlock();
		}

		data_added_[group] = false;

		cv_job_over_.notify_one();
	}
}

ParallelPipelineProcessor::~ParallelPipelineProcessor() {
	// TODO Auto-generated destructor stub
}

void ParallelPipelineProcessor::add_processor(LFPProcessor* processor) {
	Log(std::string("Adding processor ") + processor->name() + std::string(" to the parallel sub-pipeline"));
	processors_.push_back(processor);
}

void ParallelPipelineProcessor::start_workers() {
	for (unsigned int g = 0; g < NGROUP; ++g) {
		worker_threads_.push_back(new std::thread(&ParallelPipelineProcessor::process_thread, this, g));
	}
}
