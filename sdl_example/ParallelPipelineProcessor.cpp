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

	data_added_.resize(NGROUP);

	for (unsigned int g = 0; g < NGROUP; ++g) {
		worker_threads_.push_back(new std::thread(&ParallelPipelineProcessor::process_thread, this, g));
	}
}

void ParallelPipelineProcessor::process() {
	// DEBUG
	std::cout << "Set data add to 0 and notify all ... \n";

	threads_finished_ = 0;
	for (unsigned int g = 0; g < NGROUP; ++g) {
		data_added_[g] = true;
		cv_data_added_[g].notify_one();
	}

//	cv_data_added_.notify_all();

	std::unique_lock<std::mutex> lk(mtx_data_processed_);
	cv_job_over_.wait(lk, [this]{return threads_finished_ == NGROUP;});

	std::cout << "All jobs over ... \n";
}

void ParallelPipelineProcessor::process_thread(const int group) {
	while(true){
		std::unique_lock<std::mutex> lk(mtx_data_add_[group]);
		cv_data_added_[group].wait(lk, [=]{return data_added_[group];});

		// DEBUG
		std::cout << "Group " << group << " got notified about data add, pkd_id = " << buffer->last_pkg_id << "\n";
		double a = 4.5;
		double b = 0;
		for (int i = 0; i < 100000000; ++i) {
			b = sqrt(a);
			a = b * b;
		}
		std::cout << "	finished work in thread " << group << "\n";

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

