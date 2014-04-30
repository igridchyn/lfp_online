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
    
    unsigned int gaussians = 4;
    unsigned int dimensionality = 2;
        
    observations_ = arma::mat(dimensionality, 500, arma::fill::zeros);
    means_.resize(gaussians, arma::mat(dimensionality, 1));
    covariances_.resize(gaussians, arma::mat(dimensionality, dimensionality));
    weights_ = arma::vec(gaussians);
        
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
                observations_(pc, spikes_collected_) = (double)spike->pc[0][pc];
            }
            obs_spikes_.push_back(spike);
            
            spikes_collected_++;
        }
        
        // fit clusters after enough records have been collected
        //  or cluster using fitted model
        if (spikes_collected_ >= min_observations_){
            if (!gmm_fitted_){
                
                // print the PCA for clustering
//                for(int i=0;i<500;++i){
//                    printf("%f/%f\n", observations_(0, i), observations_(1,i));
//                }
                
                gmm_.Estimate(observations_);
                
                printf("\nCluster weights:");
                for (int i=0; i < 4; ++i) {
                    printf("\nprob %d = %f", i, gmm_.Weights()(i));
                }
                
                gmm_fitted_ = true;
                min_observations_ = 10;
            }

            arma::Col<size_t> labels_;
            // redraw !!
            gmm_.Classify(observations_, labels_);
            
            // TODO: assign labels to clusters; assign labels to future clusteres; redraw clusters
            // don't draw unclassified
            for (int i=0; i < labels_.size(); ++i){
                const size_t label = labels_[i];
                obs_spikes_[i]->cluster_id_ = label;
            }
            
            // classify new spikes and draw
            spikes_collected_ = 0;
            // TODO: optimize (use fixed dize)
            obs_spikes_.clear();
        }
        
        buffer->spike_buf_pos_clust_++;
    }
    
    
}