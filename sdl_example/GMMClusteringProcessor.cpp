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

mlpack::gmm::GMM<> loadGMM(const unsigned int& tetrode){
    std::string basename = "/Users/igridchyn/data/bindata/jc103/clustering/gmm_";
    const char **suffs = new const char*[10]{"0", "1", "2", "3", "4", "5", "6"};
    
    mlpack::gmm::GMM<> gmm;
    gmm.Load(basename+suffs[tetrode] + ".xml");
    
    return gmm;
}

GMMClusteringProcessor::GMMClusteringProcessor(LFPBuffer *buf, const unsigned int& min_observations, const unsigned int& rate, const unsigned int& max_clusters)
    : LFPProcessor(buf)
    , min_observations_(min_observations)
    , rate_(rate)
    , max_clusters_(max_clusters){
    
    unsigned int gaussians = 4;
    dimensionality_ = 12;

    means_.resize(buf->tetr_info_->tetrodes_number);
    covariances_.resize(buf->tetr_info_->tetrodes_number);
        
    const unsigned int ntetr =buf->tetr_info_->tetrodes_number;
        
    for (int tetr=0; tetr<buf->tetr_info_->tetrodes_number; ++tetr) {
        observations_.push_back(arma::mat(dimensionality_, (min_observations + 2) * rate_, arma::fill::zeros));
        observations_train_.push_back(arma::mat(dimensionality_, min_observations, arma::fill::zeros));
        
        means_[tetr].resize(gaussians, arma::mat(dimensionality_, 1));
        covariances_[tetr].resize(gaussians, arma::mat(dimensionality_, dimensionality_));
        weights_.push_back(arma::vec(gaussians));
    }

    total_observations_.resize(ntetr);
    gmm_fitted_.resize(ntetr);
    spikes_collected_.resize(ntetr);
        
    obs_spikes_.resize(ntetr);
    gmm_.resize(ntetr);
        
    // Fitting type by default is EMFit
    // gmm_ = mlpack::gmm::GMM<> (gaussians, dimensionality_);
        
    if (load_clustering_){
        for (int tetr=0; tetr < ntetr; ++tetr) {
            gmm_[tetr] = loadGMM(tetr);
            gmm_fitted_[tetr] = true;
        }
    }
}

mlpack::gmm::GMM<> GMMClusteringProcessor::fit_gmm(arma::mat observations_train, const unsigned int& max_clusters){
    // iterate over number of clusters
    // !!! TODO: models with non-full covariance matrix ???
    double BIC_min = (double)(1 << 30);
    mlpack::gmm::GMM<> gmm_best;
    // PROFILING
    clock_t start_all = clock();
    
    int dimensionality = observations_train.n_rows;
    
    printf("# of observations = %u\n", observations_train.n_cols);
    for (int nclust = 1; nclust <= max_clusters; ++nclust) {
        mlpack::gmm::GMM<> gmmn(nclust, dimensionality);
        
        // PROFILING
        clock_t start = clock();
        double likelihood = gmmn.Estimate(observations_train);
        double gmm_time = ((double)clock() - start) / CLOCKS_PER_SEC;
        printf("GMM time = %.1lf sec.\n", gmm_time);
        
        // = # mixing probabilities + # menas + # covariances
        int nparams = (nclust - 1) + nclust * dimensionality + nclust * dimensionality * (dimensionality + 1) / 2;
        // !!! TODO: check
        double BIC = -2 * likelihood + nparams * log(observations_train.n_cols);
        
        if (BIC < BIC_min){
            gmm_best = gmmn;
            BIC_min = BIC;
        }
        
        // DEBUG
        printf("BIC of model with full covariance and %d clusters = %lf\n", nclust, BIC);
    }
    printf("Total time for max %d clusters = %.1lf sec.\n", max_clusters, ((double)clock() - start_all)/CLOCKS_PER_SEC);
    
    return gmm_best;
}

void saveGMM(mlpack::gmm::GMM<> gmm, const unsigned int tetrode){
    std::string basename = "/Users/igridchyn/data/bindata/jc103/clustering/gmm_";
    const char **suffs = new const char*[10]{"0", "1", "2", "3", "4", "5", "6"};
    
    gmm.Save(basename+suffs[tetrode] + ".xml");
}


void GMMClusteringProcessor::process(){

    while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_){
        Spike* spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];

        // TODO: clustering for each tetrode
        
        if(spike->discarded_){
            buffer->spike_buf_pos_clust_++;
            continue;
        }
        
        const unsigned int tetr = spike->tetrode_;
        
        // if PCA has not been computed yet
        if (spike->pc == NULL){
            break;
        }
        
        for (int pc=0; pc < 3; ++pc) {
            for(int chan=0; chan < 4; ++chan){
                // TODO: disable OFB check in armadillo settings
                observations_[tetr](chan*3 + pc, total_observations_[tetr]) = (double)spike->pc[chan][pc];
            }
        }
        obs_spikes_[tetr].push_back(spike);
        total_observations_[tetr] ++;
        
        if (!gmm_fitted_[tetr]){
            if (spikes_collected_[tetr] < min_observations_){
                // TODO: use submatrix of total observations
                if (!(total_observations_[tetr] % rate_)){
                    for (int pc=0; pc < 3; ++pc) {
                        for(int chan=0; chan < 4; ++chan){
                            // TODO: disable OFB check in armadillo settings
                            observations_train_[tetr](chan*3 + pc, spikes_collected_[tetr]) = (double)spike->pc[chan][pc];
                        }
                    }

                    spikes_collected_[tetr]++;
                }
            }
            else{
                // print the PCA for clustering
                //                for(int i=0;i<500;++i){
                //                    printf("%f/%f\n", observations_(0, i), observations_(1,i));
                //                }
                
                printf("Fitting GMM for tetrode %d\n", tetr);
                gmm_[tetr] = fit_gmm(observations_train_[tetr], max_clusters_);
                printf("%ld clusters in BIC-optimal model with full covariance matrix\n", gmm_[tetr].Gaussians());
                
                // WARNING
                if (gmm_[tetr].Gaussians() == max_clusters_){
                    printf("WARNING: BIC is minimized by the maximal allowed number of clusters!\n");
                }
                
                printf("\nCluster weights:");
                for (int i=0; i < gmm_[tetr].Gaussians(); ++i) {
                    printf("\nprob %d = %f", i, gmm_[tetr].Weights()(i));
                }
                
                // TODO: !!! fit the second-level clusters
                
                gmm_fitted_[tetr] = true;
                
                if (save_clustering_){
                    saveGMM(gmm_[tetr], tetr);
                }
            }
        }else{
            // fit clusters after enough records have been collected
            //  or cluster using fitted model
            if (total_observations_[tetr] >= classification_rate_){
                arma::Col<size_t> labels_;
                // redraw !!
                gmm_[tetr].Classify(observations_[tetr].cols(0, total_observations_[tetr] - 1), labels_);
                
                // TODO: assign labels to clusters; assign labels to future clusteres; redraw clusters
                // don't draw unclassified
                for (int i=0; i < labels_.size(); ++i){
                    const size_t label = labels_[i];
                    obs_spikes_[tetr][i]->cluster_id_ = (int)label;
                }
                
                // classify new spikes and draw
                total_observations_[tetr] = 0;
                // TODO: optimize (use fixed dize)
                obs_spikes_[tetr].clear();
                observations_[tetr] = arma::mat(dimensionality_, classification_rate_, arma::fill::zeros);
            }
        }
        
        buffer->spike_buf_pos_clust_++;
    }
    
    
}