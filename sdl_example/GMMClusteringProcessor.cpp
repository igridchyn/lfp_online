//
//  GMMClusteringProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 18/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "time.h"
#include "LFPProcessor.h"
#include "mlpack/methods/gmm/gmm.hpp"

using namespace mlpack::gmm;

GMMClusteringProcessor::GMMClusteringProcessor(LFPBuffer *buf, const unsigned int& min_observations, const unsigned int& rate, const unsigned int& max_clusters)
    : LFPProcessor(buf)
    , min_observations_(min_observations)
    , rate_(rate)
    , max_clusters_(max_clusters){
    
    unsigned int gaussians = 4;
    dimensionality_ = 12;
        
    observations_ = arma::mat(dimensionality_, (min_observations + 1)* rate_, arma::fill::zeros);
    observations_train_ = arma::mat(dimensionality_, min_observations, arma::fill::zeros);

    means_.resize(gaussians, arma::mat(dimensionality_, 1));
    covariances_.resize(gaussians, arma::mat(dimensionality_, dimensionality_));
    weights_ = arma::vec(gaussians);
        
    // Fitting type by default is EMFit
    // gmm_ = mlpack::gmm::GMM<> (gaussians, dimensionality_);
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
        
        for (int pc=0; pc < 3; ++pc) {
            for(int tetr=0; tetr < 4; ++tetr){
                // TODO: disable OFB check in armadillo settings
                observations_(tetr*3 + pc, total_observations_) = (double)spike->pc[tetr][pc];
            }
        }
        obs_spikes_.push_back(spike);
        total_observations_ ++;
        
        if (!gmm_fitted_){
            if (spikes_collected_ < min_observations_){
                // TODO: use submatrix of total observations
                if (!(total_observations_ % rate_)){
                    for (int pc=0; pc < 3; ++pc) {
                        for(int tetr=0; tetr < 4; ++tetr){
                            // TODO: disable OFB check in armadillo settings
                            observations_train_(tetr*3 + pc, spikes_collected_) = (double)spike->pc[tetr][pc];
                        }
                    }

                    spikes_collected_++;
                }
            }
            else{
                // print the PCA for clustering
                //                for(int i=0;i<500;++i){
                //                    printf("%f/%f\n", observations_(0, i), observations_(1,i));
                //                }
                
                // iterate over number of clusters
                // !!! TODO: models with non-full covariance matrix ???
                double BIC_min = (double)(1 << 30);
                mlpack::gmm::GMM<> gmm_best;
                // PROFILING
                clock_t start_all = clock();
                
                for (int nclust = 1; nclust <= max_clusters_; ++nclust) {
                    mlpack::gmm::GMM<> gmmn(nclust, dimensionality_);
                    
                    // PROFILING
                    clock_t start = clock();
                    double likelihood = gmmn.Estimate(observations_train_);
                    double gmm_time = ((double)clock() - start) / CLOCKS_PER_SEC;
                    printf("GMM time = %.1lf sec.\n", gmm_time);
                    
                    int nparams = nclust * dimensionality_ * (dimensionality_ + 1);
                    // !!! TODO: check
                    double BIC = -2 * likelihood + nparams * log(observations_train_.n_cols);
                    
                    if (BIC < BIC_min){
                        gmm_best = gmmn;
                        BIC_min = BIC;
                    }
                    
                    // DEBUG
                    printf("BIC of model with full covariance and %d clusters = %lf\n", nclust, BIC);
                }
                printf("Total time for max %d clusters = %.1lf sec.\n", max_clusters_, ((double)clock() - start_all)/CLOCKS_PER_SEC);
                
                gmm_ = gmm_best;
                printf("%ld clusters in BIC-optimal model with full covariance matrix\n", gmm_.Gaussians());
                
                printf("\nCluster weights:");
                for (int i=0; i < gmm_.Gaussians(); ++i) {
                    printf("\nprob %d = %f", i, gmm_.Weights()(i));
                }
                
                gmm_fitted_ = true;
                min_observations_ = 10;
            }
        }else{
            // fit clusters after enough records have been collected
            //  or cluster using fitted model
            if (total_observations_ >= classification_rate_){
                arma::Col<size_t> labels_;
                // redraw !!
                gmm_.Classify(observations_, labels_);
                
                // TODO: assign labels to clusters; assign labels to future clusteres; redraw clusters
                // don't draw unclassified
                for (int i=0; i < labels_.size(); ++i){
                    const size_t label = labels_[i];
                    obs_spikes_[i]->cluster_id_ = (int)label;
                }
                
                // classify new spikes and draw
                total_observations_ = 0;
                // TODO: optimize (use fixed dize)
                obs_spikes_.clear();
                observations_ = arma::mat(dimensionality_, classification_rate_, arma::fill::zeros);
            }
        }
        
        buffer->spike_buf_pos_clust_++;
    }
    
    
}