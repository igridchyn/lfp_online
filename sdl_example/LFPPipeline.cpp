//
//  LFPPipeline.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPPipeline.h"
// Need processor includes to be able to construct pipeline from processor names


#include "AutocorrelogramProcessor.h"
#include "CluReaderClusteringProcessor.h"
#include "GMMClusteringProcessor.h"
#include "FetFileReaderProcessor.h"
#include "KDClusteringProcessor.h"
#include "PCAExtractionProcessor.h"
#include "PlaceFieldProcessor.h"
#include "PositionDisplayProcessor.h"
#include "SDLPCADisplayProcessor.h"
#include "SDLSignalDisplayProcessor.h"
#include "SDLWaveshapeDisplayProcessor.h"
#include "SlowDownProcessor.h"
#include "SpeedEstimationProcessor.h"
#include "SpikeDetectorProcessor.h"
#include "SwReaderProcessor.h"
#include "TransProbEstimationProcessor.h"
#include "UnitTestingProcessor.h"
#include "WhlFileReaderProcessor.h"
#include "LPTTriggerProcessor.h"
#include "FrequencyPowerBandProcessor.h"
#include "FetFileWriterProcessor.h"
#include "PackageExtractorProcessor.h"
#include "BinFileReaderProcessor.h"
#include "ParallelPipelineProcessor.h"

LFPPipeline::LFPPipeline(LFPBuffer *buf)
	:buf_(buf){
	const std::vector<std::string>& processor_names = buf->config_->processors_list_;

	std::map<std::string, unsigned int> processor_numbers;

	// starts and ends (both inclusive) of the parallel sub-pipelines
	// 		(to form processor list later)
	std::vector<unsigned int> sub_pipe_starts, sub_pipe_ends;

	for (size_t i = 0; i < processor_names.size(); ++i) {
		std::string proc_name = processor_names[i];

		if (proc_name == "Autocorrelogram"){
			processors.push_back(new AutocorrelogramProcessor(buf));
		} else if (proc_name == "CluReaderClustering"){
			processors.push_back(new CluReaderClusteringProcessor(buf));
		} else if (proc_name == "FetFileReader"){
			processors.push_back(new FetFileReaderProcessor(buf));
		} else if (proc_name == "FrequencyPowerBand"){
			processors.push_back(new FrequencyPowerBandProcessor(buf));
		} else if (proc_name == "GMMClustering"){
			processors.push_back(new GMMClusteringProcessor(buf));
		} else if (proc_name == "KDClustering"){
			processors.push_back(new KDClusteringProcessor(buf, processor_numbers[proc_name]));
		} else if (proc_name == "PackageExtractor"){
			processors.push_back(new PackageExractorProcessor(buf));
		} else if (proc_name == "PCAExtraction"){
			processors.push_back(new PCAExtractionProcessor(buf));
		} else if (proc_name == "PlaceField"){
			processors.push_back(new PlaceFieldProcessor(buf, processor_numbers[proc_name]));
		} else if (proc_name == "PositionDisplay"){
			processors.push_back(new PositionDisplayProcessor(buf));
		} else if (proc_name == "SDLControlInputMeta"){
			processors.push_back(new SDLControlInputMetaProcessor(buf, GetSDLControlInputProcessors()));
		} else if (proc_name == "SDLPCADisplay"){
			processors.push_back(new SDLPCADisplayProcessor(buf));
		} else if (proc_name == "SDLSignalDisplay"){
			processors.push_back(new SDLSignalDisplayProcessor(buf));
		} else if (proc_name == "SDLWaveshapeDisplay"){
			processors.push_back(new SDLWaveshapeDisplayProcessor(buf));
		} else if (proc_name == "SlowDown"){
			processors.push_back(new SlowDownProcessor(buf));
		} else if (proc_name == "SpeedEstimation"){
			processors.push_back(new SpeedEstimationProcessor(buf));
		} else if (proc_name == "SpikeAlignment"){
			processors.push_back(new SpikeAlignmentProcessor(buf));
		} else if (proc_name == "SpikeDetector"){
			processors.push_back(new SpikeDetectorProcessor(buf));
		} else if (proc_name == "SwReader"){
			processors.push_back(new SwReaderProcessor(buf));
		} else if (proc_name == "TransProbEstimation"){
			processors.push_back(new TransProbEstimationProcessor(buf));
		} else if (proc_name == "UnitTesting"){
			processors.push_back(new UnitTestingProcessor(buf));
		} else if (proc_name == "WaveshapeReconstruction"){
			processors.push_back(new WaveShapeReconstructionProcessor(buf));
		} else if (proc_name == "WhlFileReader"){
			processors.push_back(new WhlFileReaderProcessor(buf));
		} else if (proc_name == "LPTTrigger"){
			processors.push_back(new LPTTriggerProcessor(buf));
		} else if (proc_name == "FetFileWriter"){
			processors.push_back(new FetFileWriterProcessor(buf));
		} else if (proc_name == "BinFileReader"){
			processors.push_back(new BinFileReaderProcessor(buf));
		} else if (proc_name == "BinaryPopulationClassifier"){
			processors.push_back(new BinaryPopulationClassifierProcessor(buf));
		} else if (proc_name == "ParallelPipeline"){
			if (sub_pipe_starts.size() == sub_pipe_ends.size()){
				// the entry indicated the START of parallel sub-pipeline
				processors.push_back(new ParallelPipelineProcessor(buf));
				sub_pipe_starts.push_back(processors.size());
			} else{
				// the entry indicated the END of parallel sub-pipeline
				sub_pipe_ends.push_back(processors.size() - 1);
			}
		}
		else{
			buf->Log(std::string("ERROR: Unknown processor: ") + proc_name + ". Terminating...");
			exit(1);
		}

		processor_numbers[proc_name] ++;
	}

	// extract sub-pipelines
	if (sub_pipe_starts.size() != sub_pipe_ends.size()){
		buf->Log("ERROR: Parallel pipeline end not defined !");
		exit(534532);
	}
	for (unsigned int sp = 0; sp < sub_pipe_starts.size(); ++sp) {
		for (unsigned int p = sub_pipe_starts[sp]; p <= sub_pipe_ends[sp]; ++p){
			// the corresponding parallel processor is prior to start of the sub-pipeline
			dynamic_cast<ParallelPipelineProcessor*>(processors[sub_pipe_starts[sp] - 1])->add_processor(processors[p]);
		}
		dynamic_cast<ParallelPipelineProcessor*>(processors[sub_pipe_starts[sp] - 1])->start_workers();
	}
	// remove processors from main pipieline
	for (int sp = sub_pipe_starts.size() - 1; sp >= 0; --sp) {
		processors.erase(processors.begin() + sub_pipe_starts[sp], processors.begin() + sub_pipe_ends[sp] + 1);
	}

//	f_delays_.open(buf_->config_->getOutPath("log.path.prefix") + ".delays");

#ifdef PIPELINE_THREAD
	pipeline_thread_ = new std::thread(&LFPPipeline::process, this);
#endif
}

LFPPipeline::~LFPPipeline(){
	buf_->Log("Pipeline destructor called");

	for (size_t i=0; i < processors.size(); ++i){
		buf_->Log("Destroying processor #", i);
		delete processors[i];
	}
}

void LFPPipeline::process(){
#ifdef PIPELINE_THREAD
	while(true)
	{
		std::unique_lock<std::mutex> lk(mtx_data_add_);
		cv_data_added_.wait(lk, [this]{return data_added_;});
#endif

//		clock_t begin_time = clock();
//		unsigned int spike_ptr_before = buf_->spike_buf_pos;

		for (auto piter = processors.begin(); piter != processors.end(); ++piter) {
			(*piter)->process();
		}

//		if (buf_-> last_pkg_id > perf_delay_ && buf_->spike_buf_pos != spike_ptr_before){
//			double cycle_time = clock() - begin_time;
//			f_delays_ << cycle_time * 1000000 / CLOCKS_PER_SEC << "\n";
//
//			if (cycle_time > max_cycle_time_){
//				max_cycle_time_ = cycle_time;
//				buf_->Log("New max cycle time, us: ", cycle_time * 1000000 / CLOCKS_PER_SEC);
//			}
//		}

#ifdef PIPELINE_THREAD
		data_added_ = false;
		lk.unlock();
		cv_data_added_.notify_one();
	}
#endif
}

LFPProcessor *LFPPipeline::get_processor(const unsigned int& index){
    return processors[index];
}

std::vector<SDLControlInputProcessor *> LFPPipeline::GetSDLControlInputProcessors(){
    std::vector<SDLControlInputProcessor *> control_processors;
    
    for (size_t p=0; p<processors.size(); ++p) {
        SDLControlInputProcessor *ciproc = dynamic_cast<SDLControlInputProcessor*>(processors[p]);
        if (ciproc != nullptr){
            control_processors.push_back(ciproc);
        }
    }
    
    return control_processors;
}
