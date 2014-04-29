//
//  GMMClusteringProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 18/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "mlpack/methods/gmm/gmm.hpp"

using namespace mlpack::gmm;

GMMClusteringProcessor::GMMClusteringProcessor(LFPBuffer *buf)
    : LFPProcessor(buf)
    , min_observations_(500){
    
    observations_ = arma::mat(2, 500, arma::fill::zeros);
    means_.resize(2, arma::mat(2, 1));
    covariances_.resize(2, arma::mat(2, 2));
    weights_ = arma::vec(2);
        
    size_t gaussians = 2;
    size_t dimensionality = 2;
    
    // Fitting type by default is EMFit
    gmm_ = mlpack::gmm::GMM<> (gaussians,dimensionality);
}

void GMMClusteringProcessor::process(){

    while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_){
        Spike* spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];

        if(spike->discarded_){
            buffer->spike_buf_pos_clust_++;
            continue;
        }
        
        // if PCA has not been computed yet
        if (spike->pc == NULL){
            break;
        }
        
        if (spikes_collected_ < min_observations_){
            for (int pc=0; pc < 2; ++pc) {
                // TODO: disable OFB check in armadillo settings
                observations_(pc, spikes_collected_) = (int)spike->pc[0][pc];
            }
            
            spikes_collected_++;
        }
        
        if (!gmm_fitted_ && spikes_collected_ >= min_observations_){
            //gmm_.Estimate(observations_, means_, covariances_, weights_);
            gmm_fitted_ = true;
            
            mlpack::gmm::EMFit<> emfit_;
            emfit_.Estimate(observations_, means_, covariances_, weights_);
            
            gmm_ = mlpack::gmm::GMM<> (means_,covariances_,weights_);
            arma::Col<size_t> labels_;
            // redraw !!
            gmm_.Classify(observations_, labels_);
        
            // TODO: assign labels to clusters; assign labels to future clusteres; redraw clusters
        }
        
        buffer->spike_buf_pos_clust_++;
    }
    
    
}