//
//  UnitTestingProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 30/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "UnitTestingProcessor.h"

template<class T>
ArrayValidator<T>::ArrayValidator(std::string array_path, std::string name, T* targ_buf, int const* buf_pos_ptr, const int& gt_data_shift)
    : name_(name)
    , targ_buf_(targ_buf)
    , buf_pos_ptr_(buf_pos_ptr)
    , gt_data_shift_(gt_data_shift)
{
    std::ifstream f_filt_sig(array_path);
    f_filt_sig >> gt_data_len_;
    gt_data_ = new int[gt_data_len_];
    
    // TODO: read value of type T from file (write integrers from Matlab)
    float dum;
    for (int i=0; i<gt_data_len_; ++i) {
        f_filt_sig >> dum;
        gt_data_[i] = (T)dum;
    }
}

template<class T>
bool ArrayValidator<T>::validate(){
    // compare to all new samples in the buffer
    
    const int CHECK_DELAY = 40;
    // delay for filtes etc. - exact value is not important
    while(targ_buf_pos_ + gt_data_shift_ < gt_data_len_ && targ_buf_pos_ < *(buf_pos_ptr_) - 50){
        
        // DEBUG
        //std::cout << gt_data_[targ_buf_pos_ + gt_data_shift_] << " " << targ_buf_[targ_buf_pos_] << "\n";
        
        if (targ_buf_pos_ > CHECK_DELAY && gt_data_[targ_buf_pos_ + gt_data_shift_] != targ_buf_[targ_buf_pos_]){
            report_mismatch();
            return false;
        }
        
        targ_buf_pos_++;
    }
    
    if (!pass_reported && targ_buf_pos_ + gt_data_shift_ >= gt_data_len_){
        std::cout << "TEST PASSED: " << name_ << "\n";
        pass_reported = true;
    }
    
    return true;
}

template<class T>
void ArrayValidator<T>::report_mismatch(){
    std::cout << "\nValidation failed in " << name_ << " at buffer position " << *(buf_pos_ptr_);
}

UnitTestingProcessor::UnitTestingProcessor(LFPBuffer *buf)
		:UnitTestingProcessor(buf,
				buf->config_->getString("unit.test.dir")
				){}

UnitTestingProcessor::UnitTestingProcessor(LFPBuffer *buf, const std::string test_dir)
    : LFPProcessor(buf)
    , test_dir_(test_dir){
    // load ground-truth data of part of the signal - from text-files
    
//    const int CHANNEL = 8;
        
    // filtered signal
    // TODO configure inclusion
//    int_array_validators_.push_back(new ArrayValidator<int>(test_dir_ + "filtered.txt", "filtered", buf->filtered_signal_buf[CHANNEL], &buffer->buf_pos, 1));
//    int_array_validators_.push_back(new ArrayValidator<int>(test_dir_ + "pow.txt", "pow", buf->power_buf[CHANNEL], &buffer->buf_pos, 1));
//    int_spike_validators_.push_back(new SpikeDetectionValidator(test_dir_ + "detect.txt", "detected", buf->spike_buffer_, &buffer->spike_buf_pos, 1, buf->SPIKE_BUF_HEAD_LEN));

    t_start_ = clock();
}

void UnitTestingProcessor::process(){
    // validate arrays
    for (size_t i=0; i < int_array_validators_.size(); ++i){
        if (!int_array_validators_[i]->validate()){
            exit(1);
        }
    }
    
    for (size_t i=0; i < int_spike_validators_.size(); ++i){
        if (!int_spike_validators_[i]->validate()){
            exit(1);
        }
    }

    // report running time
//    if (buffer->last_pkg_id >= STOP_PKG){
//    	long run_time = clock();
//    	Log("==================================================================================");
//    	Log("Stop execution at pkg: ", (int)STOP_PKG);
//    	Log("Running time (sec.): ", (double)run_time / CLOCKS_PER_SEC);
//    	Log("==================================================================================");
//    	// TODO stop signalling mechanism, no rude exit
//    	exit(10);
//    }

    if (buffer->spike_buf_pos >= STOP_SPK){
       	long run_time = clock();
       	Log("==================================================================================");
       	Log("Stop execution at spike: ", (int)STOP_SPK);
       	Log("Running time (sec.): ", (double)run_time / CLOCKS_PER_SEC);
       	Log("==================================================================================");
       	// TODO stop signalling mechanism, no rude exit
       	exit(10);
       }
}

template<class T>
SpikeValidator<T>::SpikeValidator(std::string spike_path, std::string name, Spike** buf, unsigned int const *buf_pos_ptr, const int& gt_data_shift,
		const int buf_head_size)
    : name_(name)
    , targ_buf_(buf)
    , buf_pos_ptr_(buf_pos_ptr)
    , gt_data_shift_(gt_data_shift)
    , BUF_HEAD_SIZE(buf_head_size){
    
    std::ifstream f_filt_sig(spike_path);
    f_filt_sig >> gt_data_len_;
    gt_data_ = new int[gt_data_len_];
    
    float dum;
    for (int i=0; i<gt_data_len_; ++i) {
        f_filt_sig >> dum;
        gt_data_[i] = (T)dum;
    }
}

template <class T>
bool SpikeValidator<T>::validate(){
    while(targ_buf_[targ_buf_pos_ + BUF_HEAD_SIZE] != nullptr && targ_buf_pos_ < *(buf_pos_ptr_) && gt_pos_ < gt_data_len_){
        Spike *spike = targ_buf_[targ_buf_pos_ + BUF_HEAD_SIZE];
        if (spike->tetrode_ !=0){
            targ_buf_pos_++;
            continue;
        }

        if (get_feature(spike) != gt_data_[gt_pos_]){
            std::cout << "Spike validation mismatch (" << name_ << "): " << get_feature(targ_buf_[targ_buf_pos_ + BUF_HEAD_SIZE]) << " != " << gt_data_[gt_pos_] << "\n";
            return false;
        }
        targ_buf_pos_++;
        gt_pos_++;
    }
    
    if (targ_buf_pos_ >= gt_data_len_ && !pass_reported){
        std::cout << "TEST PASSED: " << name_ << "\n";
        pass_reported = true;
    }
    
    return true;
}

int SpikeDetectionValidator::get_feature(Spike *spike){
    return spike->pkg_id_;
}

std::string UnitTestingProcessor::name() {
	return "UnitTesting";
}
