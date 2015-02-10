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
#include <fstream>
#include <iostream>
#include <cstddef>
#include <assert.h>
#include <string.h>
#include <string>

class TetrodesInfo{

public:
	enum { INVALID_TETRODE = 666666 };

    // number of tetrodes = channel groups
    int tetrodes_number;
    
    // number of channels in each group
    int *channels_numbers = nullptr;
    
    // of size tetrodes_number, indices of channels in each group
	int **tetrode_channels = nullptr;
    
    // group by electrode
	int *tetrode_by_channel = nullptr;

	// maps absolute tetrode number (floor(channel number / 4))
	//	with -1 meaning tetrode is absent in the configuration
	// ASSUMPTION: channels of the same tetrode have the same integer multiple of 4
	std::vector<unsigned int> tetrode_label_map_;

    int number_of_channels(Spike* spike) const;

    static TetrodesInfo* GetInfoForTetrodesRange(const unsigned int& from, const unsigned int& to);
    static TetrodesInfo* GetMergedTetrodesInfo(const TetrodesInfo* ti1, const TetrodesInfo* ti2);

    TetrodesInfo(std::string config_path);
    TetrodesInfo();
    ~TetrodesInfo();

    bool ContainsChannel(const unsigned int& channel);
    bool ContainsChannels(const std::vector<unsigned int>& channels);

    // workaround for multiple tetrode configurations within one pipeline
    // to get tetrode index with the same channels in other config
    unsigned int Translate(const TetrodesInfo * const ti, const unsigned int& tetr);
};

#endif
