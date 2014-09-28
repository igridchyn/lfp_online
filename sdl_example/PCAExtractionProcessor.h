//
//  PCAExtractionProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_PCAExtractionProcessor_h
#define sdl_example_PCAExtractionProcessor_h

#include "LFPProcessor.h"

class PCAExtractionProcessor : public LFPProcessor{
    // projection matrix
    float ***prm = nullptr;
    const unsigned int min_samples_;
    
    void tred(float **a,int n,float d[],float e[]);
    void tqli(float d[],float e[],int n,float **z);
    void eigenc(float **m,float ev[], int ftno);
    void final(float **cor,float mea[],int ftno, int num_obj,float **prm, int prno);
    
    void compute_pcs(Spike* spike);
    
    // ??? workaround to avoid overflow
    float scale_ = 100.0f;
    
    // TODO: use online estimators
    // [channel][ws1][ws2]
    int ***cor_ = nullptr;
    // [channel][ws1]
	int **mean_ = nullptr;
	int ** sumsq_ = nullptr;
    
    // for PCA computation
	float **corf_ = nullptr;
	float *meanf_ = nullptr;
    
	float **stdf_ = nullptr;
    
    // number of objects accumulated in means / cors for each tetrode
	unsigned int *num_spikes = nullptr;
    
    // number of components per channel
    unsigned int num_pc_;
    
    // number of waveshape samples
    unsigned int waveshape_samples_;
    
    // transform matrices : from waveshape to PC
    // [channel][ws][pc]
    float ***pc_transform_;
    
    // WORKAROUND
    // TODO: recalc PCA periodically using online estimators
	bool *pca_done_ = nullptr;
    
    bool save_transform_ = false;
    bool load_transform_ = true;
    const std::string pc_path_;
    
    // for memory performance: cleanup waveshapes after getting the PCs
    bool cleanup_ws_ = false;

public:
    PCAExtractionProcessor(LFPBuffer *buffer);
    PCAExtractionProcessor(LFPBuffer *buffer, const unsigned int& num_pc, const unsigned int& waveshape_samples, const unsigned int& min_samples, const bool load_transform, const bool save_transform, const std::string& pc_path);
    virtual void process();
};

#endif
