//
//  UnitTestingProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 30/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_UnitTestingProcessor_h
#define sdl_example_UnitTestingProcessor_h

# include "LFPProcessor.h"

template<class T>
class SpikeValidator{
protected:
	const int BUF_HEAD_SIZE;

    // taget buffer to be validated
    Spike **targ_buf_;
    int targ_buf_pos_ = 0;
    unsigned int const *buf_pos_ptr_;
    
    T *gt_data_;
    int gt_data_len_;
    int gt_data_shift_;
    int gt_pos_ = 0;
    
    std::string name_;
    bool pass_reported = false;
    
    virtual T get_feature(Spike *spike) = 0;
    
public:
    SpikeValidator(std::string spike_path, std::string name, Spike** buf, unsigned int const *buf_pos_ptr, const int& gt_data_shift, const int head_size);
    virtual bool validate();

};

class SpikeDetectionValidator : public SpikeValidator<int>{
protected:
    virtual int get_feature(Spike *spike);
public:
    SpikeDetectionValidator(std::string spike_path, std::string name, Spike** buf, unsigned int const *buf_pos_ptr, const int& gt_data_shift
    		, const int head_size)
    : SpikeValidator<int>(spike_path, name, buf, buf_pos_ptr, gt_data_shift, head_size){}
};

// classes for validation of signal / spike features
template<class T>
class ArrayValidator{
    // ASSUMPTION : GT len
    
    // taget buffer to be validated
    T* targ_buf_;
    int targ_buf_pos_ = 0;
    int const *buf_pos_ptr_;
    
    T *gt_data_;
    int gt_data_len_;
    
    // of GT compared to buffer : negative => GT comes earlier : GT[i + shfit] == BUF[i]
    int gt_data_shift_;
    
    bool pass_reported = false;

    std::string name_;
    
    void report_mismatch();

public:
    // have a pointer to the buf pos, target array and internal pointer in this array
    ArrayValidator(std::string array_path, std::string name, T* buf, int const *buf_pos_ptr, const int& gt_data_shift);
    // true if all values are OK, false if ANY fails
    bool validate();
    inline bool is_over() { return targ_buf_pos_ + gt_data_shift_ >= gt_data_len_; }
};

class UnitTestingProcessor : public LFPProcessor{
    const std::string test_dir_;
    std::vector<ArrayValidator<float>*> float_array_validators_;
    std::vector<ArrayValidator<int>*> int_array_validators_;
    std::vector<SpikeValidator<int>*> int_spike_validators_;
    std::vector<SpikeValidator<float>*> float_spike_validators_;

    time_t t_start_;
    const unsigned int STOP_PKG = 500000;
    
public:
    UnitTestingProcessor(LFPBuffer *buf);
    UnitTestingProcessor(LFPBuffer *buf, const std::string test_dir);
    virtual void process();
    virtual std::string name();
};

#endif
