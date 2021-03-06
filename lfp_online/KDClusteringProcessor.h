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
#include <mutex>
#include <fstream>
#include <map>

class KDClusteringProcessor: public virtual LFPProcessor {
	// per tetrode
	std::vector<unsigned int> total_spikes_;
	std::vector<arma::fmat> observations_;

	const unsigned int MIN_SPIKES;

	const std::string BASE_PATH;

	unsigned int SAMPLING_DELAY;
	const unsigned int SAMPLING_END;

	const bool SAVE;
	const bool LOAD;

	const bool USE_PRIOR;

	const unsigned int SAMPLING_RATE;

	const float SPEED_THOLD;

	const double NN_EPS;

	const bool USE_HMM;

	const unsigned int NBINSX, NBINSY;

	const double BIN_SIZE;

	const int HMM_NEIGHB_RAD;

	unsigned int PREDICTION_DELAY;

	const unsigned int NN_K ;
	const unsigned int NN_K_COORDS;

	const unsigned int MULT_INT;
	const double SIGMA_X;
	const double SIGMA_A;
	const double SIGMA_XX;

	// delay before starting to slow down for SWR reconstruction display
	const bool SWR_SWITCH;
	unsigned int SWR_SLOWDOWN_DELAY;
	unsigned int SWR_SLOWDOWN_DURATION;
	unsigned int SWR_PRED_WIN;

	const unsigned int DUMP_DELAY;
	unsigned int DUMP_END;
	const bool DUMP_END_EXIT;

	const unsigned int HMM_RESET_RATE;

	unsigned int& spike_buf_pos_clust_;

	unsigned int THETA_PRED_WIN;

	// edge length threshold for constructing the spike graph to solve the graph cover problem
	// (0 will make it work as before computing cover consisting of all spikes
	double SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD = 0.0;
	// number of neighbours, only yo speedup, perfectly should not be reached in the spike with largest number of neighbours
	//	to decrease number of neighbours, decrease the threshold above
	unsigned int SPIKE_GRAPH_COVER_NNEIGHB = 1;

	const unsigned int FR_ESTIMATE_DELAY;

	float DUMP_SPEED_THOLD = .0;

	const bool WAIT_FOR_SPEED_EST = false;

	// if true, KDE starts after collecting MIN_SPIKES on a tetrode
	// otherwise collection will keep until the data is over
	const bool RUN_KDE_ON_MIN_COLLECTED = false;

    std::string kde_path_;

	// trees and points for spike features
	std::vector<ANNkd_tree*> kdtrees_;
	std::vector<ANNpointArray> ann_points_;

	// trees and points for spike coordinates
	std::vector<ANNkd_tree*> kdtrees_coords_;

	// for integer computations with increased precision (multiplier = MULT_INT)
	// TODO change to mat
	//std::vector< int **  > ann_points_int_;

	// knn in the train set [tetrode][point]
	std::vector<std::vector<ANNidx*> > knn_cache_;

	std::vector<bool> pf_built_;
	std::mutex kde_mutex_;
	unsigned int n_pf_built_ = 0;
	unsigned int kde_jobs_running_ = 0;

	// tmp - to estimate std
	std::vector< arma::fmat > obs_mats_;

	std::vector<unsigned int> missed_spikes_;

	// occupancy, spike occurance map, generalized firing rate
	arma::fmat pix_log_;

	std::vector<arma::fmat> pxs_;
	std::vector<arma::fmat> lxs_;


	std::vector<std::vector<arma::fmat> > laxs_;

	std::vector<std::thread*> fitting_jobs_;
	std::vector<bool> fitting_jobs_running_;

	// build p(a_i, x)
	void build_pax_(const unsigned int tetr, const unsigned int spikei, const arma::fmat& occupancy, const arma::Mat<int>& spike_coords_int);
	long long inline kern_H_ax_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr, const int& x, const int& y, const arma::Mat<int>& spike_coords_int);

	void build_lax_and_tree(const unsigned int tetr);
	void build_lax_and_tree_separate(const unsigned int tetr);

	unsigned int PRED_WIN;
	// dividable by PRED_WIN
	unsigned int last_pred_pkg_id_ = 0;
	// last prediction probabilities
	arma::fmat last_pred_probs_;

	// for current prediction window - reset after window is over
	std::vector<bool> tetr_spiked_;
	arma::fmat pos_pred_;

	// hmm prediction at last_pred_pkg_id_
	arma::fmat hmm_prediction_;

	bool sampling_end_reached_reported_ = false;
	bool delay_reached_reported = false;
	bool prediction_delay_reached_reported = false;
	bool dump_delay_reach_reported_ = false;
	bool dump_end_reach_reported_ = false;

	// SWR-params
	bool swr_regime_ = false;
	// not to start processing of the same SWR twice - memorize which one was processed last
	unsigned int last_processed_swr_start_ = 0;

	// pointer to the last processed SWR event in the buffer
	// would not rewind due to small spatial requirements to store SWR events
	unsigned int swr_pointer_ = 0;

	int npred = 0;

	// optimal trajectories TO each bin [bin][time]
	//		[bin][t] is 'best' previous bin at t-1, backtracking rule: bin_{t-1} = [bin_t][t]
	std::vector<std::vector<unsigned int> > hmm_traj_;

	// this one is assigned procces_number_-the loaded tetrode info
	TetrodesInfo* tetr_info_;

	// DUMP
	std::ofstream dec_bayesian_;
	std::ofstream debug_out_;
	std::string swr_dec_dump_path_;
	std::ofstream swr_dec_dump_;
    
	unsigned int last_hmm_reset_ = 0;

	double *pnt_;

	// continuously support a prediction matrix from window start until last spike
	const bool continuous_prediction_ = false;
	std::queue<arma::fmat> last_spike_fields_;

	unsigned int neighb_num_ = 1;
	std::vector<int> neighbour_inds_;
	std::vector<double> neighbour_dists_;

	std::vector<unsigned int> tetrode_sampling_rates_;

	int display_scale_ = 1;

	const double SWR_COMPRESSION_FACTOR;

	bool pred_dump_ = false;

	// first spike participating in current prediction
	unsigned int& spike_buf_pos_pred_start_;
	// fixed number of spikes for prediction window; 0 if no limit
	const unsigned int prediction_window_spike_number_ = 0;
	// packages for fixed length, number of spikes for fixed number of spikes
	const unsigned int prediction_windows_overlap_ = 0;

	bool BINARY_CLASSIFIER = false;

	const unsigned int MAX_KDE_JOBS = 5;

	const bool SINGLE_PRED_PER_SWR = false;

	const double POS_OOR_LIMIT = .003;

	const bool IGNORE_LX;

	// use interleaving windows : odd - for training, even - for validation
	const bool INTERLEAVING_WINDOWS;

	// minimal number of position samples required to bulid the model
	const unsigned int MIN_POS_SAMPLES;

	const double KD_MIN_OCC;

	// spikes not used for current prediction for various reasons (i.e. distance from pivot spikes too large)
	std::vector<unsigned int> prediction_skipped_spikes_;
	std::vector<unsigned int> window_spikes_;

	std::vector<bool> use_tetrode_;

	// number of spikes ignored in the current prediction
	unsigned int pred_spike_ignored_ = 0;

	// ANALYSIS
	int an_last_dumped_swr_before_ = -1;
	int an_last_dumped_swr_after_ = -1;

	void update_hmm_prediction();
	void reset_hmm();
	void dump_hmm_prediction();

	void load_laxs_tetrode(unsigned int tetrode);
	void dump_positoins_if_needed(const unsigned int& mx, const unsigned int& my);
	void dump_swr_window_spike_count();
	void dump_prediction_if_needed(unsigned int groupid);
	void validate_prediction_window_bias();

public:
	KDClusteringProcessor(LFPBuffer *buf, const unsigned int& processor_number);

	KDClusteringProcessor(LFPBuffer *buf, const unsigned int& processor_number, const unsigned int num_spikes, const std::string base_path,
			const unsigned int sampling_delay, const bool save, const bool load,
			const bool use_prior, const unsigned int sampling_rate, const float speed_thold,
			const float eps, const bool use_hmm, const unsigned int NBINS, const unsigned int bin_size, const int neighb_rad,
			const unsigned int prediction_delay, const unsigned int nn_k, const unsigned int nn_k_coords,
			const unsigned int mult_int,
			const double sigma_x, const double sigma_a, const double sigma_xx);
	virtual ~KDClusteringProcessor();

	const arma::fmat& GetPrediction();
	void JoinKDETasks();

	// LFPProcessor
	virtual void process();
	virtual std::string name();
};

#endif /* KDCLUSTERINGPROCESSOR_H_ */

