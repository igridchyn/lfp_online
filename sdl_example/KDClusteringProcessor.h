/*
 * KDClusteringProcessor.h
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#ifndef KDCLUSTERINGPROCESSOR_H_
#define KDCLUSTERINGPROCESSOR_H_

#include "LFPProcessor.h"
#include "PlaceFieldProcessor.h"
#include <armadillo>
#include <ANN/ANN.h>
#include <thread>
#include <fstream>

class KDClusteringProcessor: public LFPProcessor {
	// per tetrode
	std::vector<unsigned int> total_spikes_;
	std::vector<arma::mat> observations_;
	std::vector<std::vector<Spike *> > obs_spikes_;

	const unsigned int MIN_SPIKES;

	// TODO: configurableize
	const unsigned int DIM = 12;

	// TODO: parametrize (from main for a start)
	const unsigned int NN_K ;
	const unsigned int NN_K_COORDS;
	const double NN_EPS = 0.1;
	const unsigned int NBINS;
	// TODO float
	const unsigned int BIN_SIZE;

	// normalized to have average feature std / per tetrode
	std::vector<arma::Mat<int>> coords_normalized_;

	// TODO: test for integer overflow in KDE operations
	const unsigned int MULT_INT;
	const double SIGMA_X;
	const double SIGMA_A;
	const double SIGMA_XX;

	const bool SAVE;
	const bool LOAD;
	const std::string BASE_PATH;

	const bool USE_PRIOR;
	const unsigned int PREDICTION_DELAY;

	const unsigned int SAMPLING_RATE;
	const unsigned int SAMPLING_DELAY;
	const float SPEED_THOLD;

	const bool USE_HMM;
	const float LX_WEIGHT;
	const float HMM_TP_WEIGHT;

	// TODO: !!! take from tetrode info? (channel nu,bner * 3 ???)
	const unsigned int N_FEAT = 12;

	// trees and points for spike features
	std::vector<ANNkd_tree*> kdtrees_;
	std::vector<ANNpointArray> ann_points_;

	// trees and points for spike coordinates
	std::vector<ANNkd_tree*> kdtrees_coords_;

	// for integer computations with increased precision (multiplier = MULT_INT)
	// TODO change to mat
	std::vector< int **  > ann_points_int_;

	// knn in the train set [tetrode][point]
	std::vector<std::vector<ANNidx*> > knn_cache_;

	// p(a, x) for each point in the set ( ? + medians between nearest neighbours ?)
	// !!! indexing should be the same as in obs_spikes_
	std::vector<std::vector<arma::mat> > spike_place_fields_;

	std::vector<bool> pf_built_;
	unsigned int n_pf_built_ = 0;

	// tmp - to estimate std
	std::vector< arma::mat > obs_mats_;

	std::vector<int> missed_spikes_;

	// occupancy, spike occurance map, generalized firing rate
	arma::mat pix_log_;
	arma::mat pix_;

	std::vector<arma::mat> pxs_;
	std::vector<arma::mat> lxs_;


	std::vector<std::vector<arma::mat> > laxs_;

	std::vector<std::thread*> fitting_jobs_;
	std::vector<bool> fitting_jobs_running_;

	// build p(a_i, x)
	void build_pax_(const unsigned int tetr, const unsigned int spikei, const arma::mat& occupancy, const arma::Mat<int>& spike_coords_int);
	long long inline kern_H_ax_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr, const int& x, const int& y, const arma::Mat<int>& spike_coords_int);

	// TODO synchronize pix dumping
	void build_lax_and_tree(const unsigned int tetr);
	void build_lax_and_tree_separate(const unsigned int tetr);

	// to get the place fields
	// TODO interface and implementation - OccupancyProvider
	PlaceFieldProcessor *pfProc_;

	unsigned int PRED_WIN = 2000;
	// dividable by PRED_WIN
	unsigned int last_pred_pkg_id_ = 0;;
	// last prediction probabilities
	// TODO array of predictions if more than 1 window available
	arma::mat last_pred_probs_;

	// for current prediction window - reset after window is over
	std::vector<bool> tetr_spiked_;
	arma::mat pos_pred_;

	// hmm prediction at last_pred_pkg_id_
	arma::mat hmm_prediction_;
	const int HMM_NEIGHB_RAD;

	bool delay_reached_reported = false;
	bool prediction_delay_reached_reported = false;

	// SWR-params
	bool swr_regime_ = false;
	// not to start processing of the same SWR twice - memorize which one was processed last
	unsigned int last_processed_swr_start_ = 0;
	unsigned int SWR_SLOWDOWN_DELAY = 23 * 1000000;
	const bool SWR_SWITCH = false;
	const unsigned int HMM_RESET_RATE = 60 * 1000000;

	// STATS / DEBUG
	std::ofstream dist_theta_;
	std::ofstream dist_swr_;
	std::ofstream err_bay_;
	std::ofstream err_hmm_;
	std::ofstream dec_coords_;
	int npred = 0;

	// optimal trajectories TO each bin [bin][time]
	//		[bin][t] is 'best' previous bin at t-1, backtracking rule: bin_{t-1} = [bin_t][t]
	std::vector<std::vector<int> > hmm_traj_;

	void update_hmm_prediction();
	void reset_hmm();

	void load_laxs_tetrode(unsigned int tetrode);

public:
	KDClusteringProcessor(LFPBuffer *buf);

	KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes, const std::string base_path,
			const unsigned int sampling_delay, const bool save, const bool load,
			const bool use_prior, const unsigned int sampling_rate, const float speed_thold,
			const float eps, const bool use_hmm, const unsigned int NBINS, const unsigned int bin_size, const int neighb_rad,
			const unsigned int prediction_delay, const unsigned int nn_k, const unsigned int nn_k_coords,
			const unsigned int mult_int,
			const double sigma_x, const double sigma_a, const double sigma_xx);
	virtual ~KDClusteringProcessor();

	const arma::mat& GetPrediction();
	void JoinKDETasks();

	// LFPProcessor
	virtual void process();
};

#endif /* KDCLUSTERINGPROCESSOR_H_ */
