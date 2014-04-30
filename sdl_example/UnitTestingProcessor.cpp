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
    
    float dum;
    for (int i=0; i<gt_data_len_; ++i) {
        f_filt_sig >> dum;
        gt_data_[i] = (int)dum;
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

UnitTestingProcessor::UnitTestingProcessor(LFPBuffer *buf, const std::string test_dir)
    : LFPProcessor(buf)
    , test_dir_(test_dir){
    // load ground-truth data of part of the signal - from text-files
    
    const int CHANNEL = 8;
        
    // filtered signal
        int_array_validators_.push_back(new ArrayValidator<int>(test_dir_ + "filtered.txt", "filtered", buf->filtered_signal_buf[CHANNEL], &buffer->buf_pos, 1));
}

void UnitTestingProcessor::process(){
    // validate arrays
    for (int i=0; i < int_array_validators_.size(); ++i){
        if (!int_array_validators_[i]->validate()){
            exit(1);
        }
    }
}