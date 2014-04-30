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
    int gt_data_shift;

    std::string name_;
    
public:
    // have a pointer to the buf pos, target array and internal pointer in this array
    ArrayValidator(std::string array_path, std::string name, T* buf, int const *buf_pos_ptr);
    // true if all values are OK, false if ANY fails
    bool validate();
};

class UnitTestingProcessor : public LFPProcessor{
    const std::string test_dir_;
    std::vector<ArrayValidator<float>*> float_array_validators_;
    std::vector<ArrayValidator<int>*> int_array_validators_;
    
public:
    UnitTestingProcessor(LFPBuffer *buf, const std::string test_dir);
    virtual void process();
};

#endif
