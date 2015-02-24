//
//  GMMClusteringProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 18/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "time.h"

#include "LFPProcessor.h"
#include "GMMClusteringProcessor.h"

using namespace mlpack::gmm;

mlpack::gmm::GMM<> GMMClusteringProcessor::loadGMM(const unsigned int& tetrode, const std::string& gmm_path_basename){
    mlpack::gmm::GMM<> gmm;

    std::string gmm_file_name = gmm_path_basename + Utils::NUMBERS[tetrode] + ".xml";
    if (Utils::FS::FileExists(gmm_file_name)){
    	gmm.Load(gmm_file_name);
    	gmm_fitted_[tetrode] = true;

    	// TODO: filter out tiny clusters after clustering

    	std::vector<arma::vec> means;
    	std::vector<arma::mat> covs;
    	arma::vec weights;

    	std::cout << "Cluster weights: ";
    	const double EPS = 0.00001;
    	int ngauss = 0;
    	for (size_t c = 0; c < gmm.Gaussians(); ++c) {
    		double w = gmm.Weights()(c);
    		std::cout << w << " ";

    		if (w > EPS){
    			ngauss ++;
    			means.push_back(gmm.Means()[c]);
    			covs.push_back(gmm.Covariances()[c]);
    			weights.resize(weights.size() + 1);
    			weights(ngauss - 1) = w;
    		}
		}
    	std::cout << "\n";

    	gmm = mlpack::gmm::GMM<>(means, covs, weights);
    	buffer->Log(std::string("Number of clusters after filtering: ") + Utils::NUMBERS[gmm.Gaussians()]);
    }
    else{
    	buffer->Log(std::string("No GMM for tetrode ") + Utils::NUMBERS[tetrode]);
    }

    if (gmm.Gaussians() == 0){
    	gmm_fitted_[tetrode] = false;
    }

    return gmm;
}

GMMClusteringProcessor::GMMClusteringProcessor(LFPBuffer* buf)
	:GMMClusteringProcessor(buf,
			buf->config_->getInt("gmm.min.observations"),
			buf->config_->getInt("gmm.rate"),
			buf->config_->getInt("gmm.max.clusters"),
			buf->config_->getBool("gmm.load"),
			buf->config_->getBool("gmm.save", ! buf->config_->getBool("gmm.load")),
			buf->config_->getOutPath("gmm.path.base")
			)
	{
}

GMMClusteringProcessor::GMMClusteringProcessor(LFPBuffer *buf, const unsigned int& min_observations, const unsigned int& rate,
		const unsigned int& max_clusters, const bool load_model, const bool save_model, const std::string& gmm_path_base)
    : LFPProcessor(buf)
    , min_observations_(min_observations)
    , rate_(rate)
    , max_clusters_(max_clusters)
    , save_clustering_(save_model)
    , load_clustering_(load_model)
	, gmm_path_basename_(gmm_path_base)
	, num_princomp_(buf->config_->getInt("gmm.num.princomp", 3))
	, nclust_step_(buf->config_->getInt("gmm.nclust.step", 1)){
    
    const unsigned int ntetr =buf->tetr_info_->tetrodes_number();
        
    for (size_t tetr=0; tetr<buf->tetr_info_->tetrodes_number(); ++tetr) {
    	unsigned int dimensionality = buffer->tetr_info_->channels_number(tetr) * num_princomp_;

        // more spikes will be collected while clustering happens
        observations_.push_back(arma::mat(dimensionality, 2 * min_observations * rate_, arma::fill::zeros));
        observations_train_.push_back(arma::mat(dimensionality, min_observations, arma::fill::zeros));
     }

    total_observations_.resize(ntetr);
    spikes_collected_.resize(ntetr);
        
    obs_spikes_.resize(ntetr);
    gmm_.resize(ntetr);
    gmm_fitted_ = new bool[ntetr];
        
    // Fitting type by default is EMFit
    // gmm_ = mlpack::gmm::GMM<> (gaussians, dimensionality_);
        
    if (load_clustering_){
        for (unsigned int tetr=0; tetr < ntetr; ++tetr) {
            gmm_[tetr] = loadGMM((unsigned int)tetr, gmm_path_base);
            
            buffer->Log(std::string("Loaded GMM with ") + Utils::NUMBERS[gmm_[tetr].Gaussians()] + " clusters for tetrode " + Utils::NUMBERS[tetr] + "\n\n");

            // resize population vector in window buffer
            buf->population_vector_window_[tetr].resize(gmm_[tetr].Gaussians() + 1);
        }
    }

    clustering_jobs_.resize(ntetr);
    clustering_job_running_.resize(ntetr);
}

void GMMClusteringProcessor::fit_gmm_thread(const unsigned int& tetr){
    // iterate over number of clusters
    // !!! TODO: models with non-full covariance matrix ???
    double BIC_min = (double)(1 << 30);
    mlpack::gmm::GMM<> gmm_best;
    // PROFILING
    clock_t start_all = clock();
    
    arma::mat& observations_train = observations_train_[tetr];

    int dimensionality = observations_train.n_rows;
    
    printf("# of observations = %u\n", observations_train.n_cols);
    for (unsigned int nclust = 1; nclust <= max_clusters_; nclust += nclust_step_) {
        mlpack::gmm::GMM<> gmmn(nclust, dimensionality);

        // PROFILING
        clock_t start = clock();
        double likelihood = std::numeric_limits<double>::infinity();
        try{
        	likelihood = gmmn.Estimate(observations_train);
        }
        catch(...){
        	buffer->Log(std::string("Exception occurred during GMM fit for tetrode #") + Utils::NUMBERS[tetr] + " with " + Utils::NUMBERS[nclust] + " clusters");
        }

        double gmm_time = ((double)clock() - start) / CLOCKS_PER_SEC;
        printf("GMM time = %.1lf sec.\n", gmm_time);
        
        // = # mixing probabilities + # means + # covariances
        int nparams = (nclust - 1) + nclust * dimensionality + nclust * dimensionality * (dimensionality + 1) / 2;
        // !!! TODO: check
        double BIC = -2 * likelihood + nparams * log(dimensionality);
        
        if (BIC < BIC_min){
            gmm_best = gmmn;
            BIC_min = BIC;
        }
        
        // DEBUG
        printf("  Tetr. #%d: BIC of model with full covariance and %d clusters = %lf\n", tetr, nclust, BIC);
    }
    printf("Total time for max %d clusters = %.1lf sec.\n", max_clusters_, ((double)clock() - start_all)/CLOCKS_PER_SEC);

    gmm_fitted_[tetr] = true;
    gmm_[tetr] = gmm_best;
    buffer->population_vector_window_[tetr].resize(gmm_[tetr].Gaussians() + 1);

    if (save_clustering_){
        saveGMM(gmm_[tetr], tetr);
    }else{
    	buffer->Log("GMM model has not been save due to configuration");
    }
    
    printf("%ld clusters in BIC-optimal model with full covariance matrix (tetrode #%d)\n", gmm_[tetr].Gaussians(), tetr);
    
    // WARNING
    if (gmm_[tetr].Gaussians() == max_clusters_){
        printf("WARNING: BIC is minimized by the maximal allowed number of clusters!\n");
    }
    
    printf("Cluster weights:");
    for (unsigned int i=0; i < gmm_[tetr].Gaussians(); ++i) {
        printf("\nprob %d = %f", i, gmm_[tetr].Weights()(i));
    }
    std::cout << "\n";
}

void GMMClusteringProcessor::saveGMM(mlpack::gmm::GMM<> gmm, const unsigned int tetrode){
    gmm.Save(gmm_path_basename_ + Utils::NUMBERS[tetrode] + ".xml");
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
        const unsigned int nchan = buffer->tetr_info_->channels_number(tetr);
        
        // if PCA has not been computed yet
        if (spike->pc == nullptr){
            break;
        }
        
        for(unsigned int fet=0; fet < buffer->feature_space_dims_[tetr]; ++fet){
        	// TODO: disable OFB check in armadillo settings
        	if (observations_[tetr].n_cols <= total_observations_[tetr]){
        		observations_[tetr].resize(observations_[tetr].n_rows, observations_[tetr].n_cols * 2);
        	}

        	observations_[tetr](fet, total_observations_[tetr]) = (double)spike->pc[fet];
        }

        obs_spikes_[tetr].push_back(spike);
        total_observations_[tetr] ++;
        
        if (!gmm_fitted_[tetr]){
            if (spikes_collected_[tetr] < min_observations_){
                // TODO: use submatrix of total observations
                if (!(total_observations_[tetr] % rate_)){

                	for(unsigned int fet=0; fet < buffer->feature_space_dims_[tetr]; ++fet){
                		// TODO: disable OFB check in armadillo settings
                		observations_train_[tetr](fet, spikes_collected_[tetr]) = (double)spike->pc[fet];
                	}

                    spikes_collected_[tetr]++;
                }
            }
            else{
                // print the PCA for clustering
                //                for(int i=0;i<500;++i){
                //                    printf("%f/%f\n", observations_(0, i), observations_(1,i));
                //                }
                
                // if job not started, start, otherwise it is still running
                if (!clustering_job_running_[tetr]){
                
                    printf("Start job: Fitting GMM for tetrode %d\n", tetr);
                    clustering_jobs_[tetr] = new std::thread(&GMMClusteringProcessor::fit_gmm_thread, this, tetr);
                    clustering_job_running_[tetr] = true;
                }
            }
        }else{

        	bool first_class_after_clust = false;

            // if job was running but now it is over (because gmm_fitted_ == true
            if (clustering_job_running_[tetr]){
                clustering_job_running_[tetr] = false;
                
                clustering_jobs_[tetr]->join();

                // TODO: !!! fit the second-level clusters
                
                gmm_fitted_[tetr] = true;
                first_class_after_clust = true;
            }

            // fit clusters after enough records have been collected
            //  or cluster using fitted model
            if (total_observations_[tetr] >= classification_rate_){
                arma::Col<size_t> labels_;
                // redraw !!
                gmm_[tetr].Classify(observations_[tetr].cols(0, total_observations_[tetr] - 1), labels_);
                
                // UPDATE window population vector in BUFFER

                // 2nd level clustering if classification was called first time after model was built
                if (first_class_after_clust){
                	// re-cluster units with bad autocorrelation and high firing rate
                	for (size_t clust = 0; clust < gmm_[tetr].Gaussians(); ++clust) {
                		float clust_rate = buffer->SAMPLING_RATE / (obs_spikes_[tetr][labels_.size() - 1]->pkg_id_) * gmm_[tetr].Weights()[clust] * total_observations_[tetr];
                		if (clust_rate > 40){
                			// 2nd iteration of clustering
                			// TODO !!! implement 2nd iteration
                		}
					}
                }

                // TODO: assign labels to clusters; assign labels to future clusteres; redraw clusters
                // don't draw unclassified
                for (size_t i=0; i < labels_.size(); ++i){
                    const size_t label = labels_[i];
                    obs_spikes_[tetr][i]->cluster_id_ = (int)label;
                    // done in Spike alignment (cheap operation anyway)
                    // buffer->UpdateWindowVector(obs_spikes_[tetr][i]);
                    buffer->cluster_spike_counts_(tetr, label) += 1;
                }
                
                // classify new spikes and draw
                total_observations_[tetr] = 0;
                // TODO: optimize (use fixed dize)
                obs_spikes_[tetr].clear();
                observations_[tetr] = arma::mat(num_princomp_ * nchan, classification_rate_, arma::fill::zeros);
            }
        }
        
        buffer->spike_buf_pos_clust_++;
    }
}

void GMMClusteringProcessor::JoinGMMTasks(){
    for(size_t t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
        if(clustering_job_running_[t])
            clustering_jobs_[t]->join();
    }
    std::cout << "All running GMM finished...\n";
}

