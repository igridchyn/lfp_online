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
#include <vector>

class TetrodesInfo{

public:
    // number of tetrodes = channel groups
    int tetrodes_number;
    
    // number of channels in each group
    int *channels_numbers = NULL;
    
    // of size tetrodes_number, indices of channels in each group
	int **tetrode_channels = NULL;
    
    // group by electrode
	int *tetrode_by_channel = NULL;
    
    int number_of_channels(Spike* spike);

    static TetrodesInfo* GetInfoForTetrodesRange(const unsigned int& from, const unsigned int& to);
    static TetrodesInfo* GetMergedTetrodesInfo(const TetrodesInfo* ti1, const TetrodesInfo* ti2);

    TetrodesInfo(std::string config_path);
    TetrodesInfo();
    ~TetrodesInfo();

    bool ContainsChannel(const unsigned int& channel);
    bool ContainsChannels(const std::vector<unsigned int>& channels);
};

#endif
