//
//  TetrodesInfo.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_TetrodesInfo_h
#define sdl_example_TetrodesInfo_h

#include "Spike.h"

class TetrodesInfo{
public:
    // number of tetrodes = channel groups
    int tetrodes_number;
    
    // number of channels in each group
    int *channels_numbers;
    
    // of size tetrodes_number, indices of channels in each group
    int **tetrode_channels;
    
    // group by electrode
    int *tetrode_by_channel;
    
    int number_of_channels(Spike* spike);

    static TetrodesInfo* GetInfoForTetrodesRange(const unsigned int& from, const unsigned int& to);
};

#endif
