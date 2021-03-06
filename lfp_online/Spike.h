//
//  Spike.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_Spike_h
#define sdl_example_Spike_h

#include <cmath>

// NOTES:
// spike identification is based on unique number assigned to each detected spike
// it is assumed that fraction of spikes discaeded after initial power threshold + refractory detection is negligible

typedef short ws_type ;

// object size:
class Spike{
public:
    static const int WL_LENGTH = 22;
    static const int WS_LENGTH_ALIGNED = 32;

    unsigned int pkg_id_ = 0;
    ws_type **waveshape = nullptr;
    int **waveshape_final = nullptr;

    // FEATURES
    float *pc = nullptr;
    float peak_to_valley_1_ = 0;
    float peak_to_valley_2_ = 0;
    float intervalley_ = 0;
    // power of a filter at the peak
    float power_ = 0;

    float **extra_features_ = nullptr;

    int cluster_id_ = -1;

    int tetrode_ = -1;
    int num_channels_ = -1;

    bool discarded_ = false;

    // coordinates
    float x = nanf(""), y = nanf("");
    float speed = nanf("");

    // number of channel with the peak
    unsigned char peak_chan_ = 255;

    unsigned int num_pc_ = 0;

    Spike();

    Spike(int pkg_id, int tetrode);
    // to optimize for speed spike objects in buffer are reinitialized instead of deleting and creating new
    void init(int pkg_id, int tetrode);

    float getWidth(float level, int chan);
    bool crossesWaveShapeFinal(unsigned int channel, float x1, float y1, float x2, float y2);
    bool crossesWaveShapeReconstructed(unsigned int channel, float x1, float y1, float x2, float y2);

    void find_one_peak(int *ptmout,int peakp,int peakgit,int *ptmval);
    void find_valleys(int ptm, int ptv);
    void set_peak_valley_features();

    const float& getFeature(const unsigned int& index) const;
    float* getFeatureAddr(const unsigned int& index);

    void assignExtraFeaturePointers();

    ~Spike();
};

#endif
