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

GMMClusteringProcessor::GMMClusteringProcessor(LFPBuffer *buf, const unsigned int& min_observations)
    : LFPProcessor(buf)
    , min_observations_(min_observations){
    
    unsigned int gaussians = 4;
    dimensionality_ = 2;
        
    observations_ = arma::mat(dimensionality_, min_observations, arma::fill::zeros);
    means_.resize(gaussians, arma::mat(dimensionality_, 1));
    covariances_.resize(gaussians, arma::mat(dimensionality_, dimensionality_));
    weights_ = arma::vec(gaussians);
        
    // Fitting type by default is EMFit
    gmm_ = mlpack::gmm::GMM<> (gaussians, dimensionality_);
}

void GMMClusteringProcessor::process(){

    while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_){
        Spike* spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];

        // TODO: clustering for each tetrode
        if(spike->discarded_ || spike->tetrode_ > 0){
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
                
                // iterate over number of clusters
                // !!! TODO: models with non-full covariance matrix ???
                double BIC_min = 10000000;
                mlpack::gmm::GMM<> gmm_best;
                for (int nclust = 1; nclust < 10; ++nclust) {
                    mlpack::gmm::GMM<> gmmn(nclust, 2);
                    double likelihood = gmmn.Estimate(observations_);
                    int nparams = nclust * dimensionality_ * (dimensionality_ + 1);
                    double BIC = -2 * likelihood + nparams * log(observations_.size());
                    
                    if (BIC < BIC_min){
                        gmm_best = gmmn;
                        BIC_min = BIC;
                    }
                    
                    // DEBUG
                    printf("BIC of model with full covariance and %d clusters = %lf\n", nclust, BIC);
                }
                
                gmm_ = gmm_best;
                printf("%ld clusters in BIC-optimal model with full covariance matrix\n", gmm_.Gaussians());
                
                printf("\nCluster weights:");
                for (int i=0; i < gmm_.Gaussians(); ++i) {
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