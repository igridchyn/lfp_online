//
//  Spike.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_Spike_h
#define sdl_example_Spike_h


// NOTES:
// spike identification is based on unique number assigned to each detected spike
// it is assumed that fraction of spikes discaeded after initial power threshold + refractory detection is negligible

// object size:
class Spike{
public:
    static const int WL_LENGTH = 22;
    static const int WS_LENGTH_ALIGNED = 32;
    
    int pkg_id_;
    int **waveshape = NULL;
    int **waveshape_final = NULL;
    float **pc = NULL;
    
    int cluster_id_ = -1;
    
    int tetrode_;
    int num_channels_;
    
    Spike(int pkg_id, int tetrode);
    
    // workaround ? - has to be checked in every processor
    // TODO: list of spikes or new buffer
    bool discarded_ = false;
    
    // for next processors to know whether they can process this spike
    bool aligned_ = false;
    
    // coords
    float x, y;
    float speed;
    
};

#endif