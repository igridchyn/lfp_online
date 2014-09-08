//
//  GMMClusteringProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_GMMClusteringProcessor_h
#define sdl_example_GMMClusteringProcessor_h

#include "mlpack/methods/gmm/gmm.hpp"
#include <thread>

// TODO: create abstract clustering class
class GMMClusteringProcessor : public LFPProcessor{
    unsigned int dimensionality_;
    unsigned int rate_;
    unsigned int max_clusters_;
    int min_observations_;
    
    bool save_clustering_ = false;
    bool load_clustering_ = true;
    
    // classify every .. spikes (to reduce computations overhead)
    static const int classification_rate_ = 1;
    
    std::vector<unsigned int> total_observations_;
    std::vector<arma::mat> observations_;
    std::vector<arma::mat> observations_train_;
    
    std::vector<std::vector<Spike*> > obs_spikes_;
    
    std::vector< mlpack::gmm::GMM<> > gmm_;
    
    // whether thread clustering job is over and can be joined
    bool *gmm_fitted_;
    std::vector<unsigned int> spikes_collected_;
    
    std::vector<std::vector< arma::vec > > means_;
    std::vector<std::vector< arma::mat > > covariances_;
    
    std::vector<arma::vec> weights_;
    
    // array of jobs [one per tetrode] and their start flags
    std::vector<std::thread*> clustering_jobs_;
    std::vector<bool> clustering_job_running_;
    
    mlpack::gmm::GMM<> fit_gmm(arma::mat observations_train, const unsigned int& max_clusters);
    
    const std::string gmm_path_basename_;

    void saveGMM(mlpack::gmm::GMM<> gmm, const unsigned int tetrode);
    mlpack::gmm::GMM<> loadGMM(const unsigned int& tetrode, const std::string& gmm_path_basename);
    
    void fit_gmm_thread(const unsigned int& tetr);
    void gmm_task(int *ptr);
    
public:
    GMMClusteringProcessor(LFPBuffer* buf);
    GMMClusteringProcessor(LFPBuffer* buf, const unsigned int& min_observations, const unsigned int& rate, const unsigned int& max_clusters, const bool load_model, const bool save_model, const std::string& gmm_path_base);
    
    // LFPProcessor
    virtual void process();
    
    void JoinGMMTasks();
};

#endif
