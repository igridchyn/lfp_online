/*
 * KDClusteringProcessor.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#include "KDClusteringProcessor.h"

#include <fstream>

void KDClusteringProcessor::load_laxs_tetrode(unsigned int t){
	std::cout << "Load probability estimations for tetrode " << t << "...\n";

	// load l(a,x) for all spikes of the tetrode
	//			for (int i = 0; i < MIN_SPIKES; ++i) {
	//				laxs_[t][i].load(BASE_PATH + Utils::NUMBERS[t] + "_" + Utils::Converter::int2str(i) + ".mat");
	//			}

	// load binary combined matrix and extract individual l(a,x)
	arma::mat laxs_tetr_(NBINS, NBINS * MIN_SPIKES);
	laxs_tetr_.load(BASE_PATH + Utils::NUMBERS[t] + "_tetr.mat");
	for (int s = 0; s < MIN_SPIKES; ++s) {
		laxs_[t][s] = laxs_tetr_.cols(s*NBINS, (s + 1) * NBINS - 1);
	}
	//laxs_tetr_.save(BASE_PATH + Utils::NUMBERS[t] + "_tetr.mat");

	// load marginal rate function
	lxs_[t].load(BASE_PATH + Utils::NUMBERS[t] + "_lx.mat");
	double maxval = arma::max(arma::max(lxs_[t]));
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			if (lxs_[t](xb, yb) == 0){
				lxs_[t](xb, yb) = maxval;
			}
		}
	}

	pxs_[t].load(BASE_PATH + Utils::NUMBERS[t] + "_px.mat");

	pf_built_[t] = true;

	std::ifstream kdtree_stream(BASE_PATH + Utils::NUMBERS[t] + ".kdtree");
	kdtrees_[t] = new ANNkd_tree(kdtree_stream);
	kdtree_stream.close();
}

KDClusteringProcessor::KDClusteringProcessor(LFPBuffer* buf)
	: KDClusteringProcessor(buf,
			buf->config_->getInt("kd.min.spikes"),
			buf->config_->getOutPath("kd.path.base"),
			buf->config_->getInt("kd.sampling.delay"),
			buf->config_->getBool("kd.save"),
			!buf->config_->getBool("kd.save"),
			buf->config_->getBool("kd.use.prior"),
			buf->config_->getInt("kd.sampling.rate"),
			buf->config_->getFloat("kd.speed.thold"),
			buf->config_->getFloat("kd.nn.eps"),
			buf->config_->getBool("kd.use.hmm"),
			buf->config_->getInt("nbins"),
			buf->config_->getInt("bin.size"),
			buf->config_->getInt("kd.hmm.neighb.rad"),
			buf->config_->getInt("kd.prediction.delay"),
			buf->config_->getInt("kd.nn.k"),
			buf->config_->getInt("kd.nn.k.space"),
			buf->config_->getInt("kd.mult.int"),
			buf->config_->getFloat("kd.sigma.x"),
			buf->config_->getFloat("kd.sigma.a"),
			buf->config_->getFloat("kd.sigma.xx")
			){
}


KDClusteringProcessor::KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes,
		const std::string base_path,
		const unsigned int sampling_delay, const bool save, const bool load, const bool use_prior,
		const unsigned int sampling_rate, const float speed_thold, const float eps,
		const bool use_hmm, const unsigned int nbins, const unsigned int bin_size, const int neighb_rad,
		const unsigned int prediction_delay, const unsigned int nn_k, const unsigned int nn_k_coords,
		const unsigned int mult_int,
		const double sigma_x, const double sigma_a, const double sigma_xx)
	: LFPProcessor(buf)
	, MIN_SPIKES(num_spikes)
	//, BASE_PATH("/hd1/data/bindata/jc103/jc84/jc84-1910-0116/pf_ws/pf_"){
	, BASE_PATH(base_path)
	, SAMPLING_DELAY(sampling_delay)
	, SAVE(save)
	, LOAD(load)
	, USE_PRIOR(use_prior)
	, SAMPLING_RATE(sampling_rate)
	, SPEED_THOLD(speed_thold)
	, NN_EPS(eps)
	, USE_HMM(use_hmm)
	, NBINS(nbins)
	, BIN_SIZE(bin_size)
	, HMM_NEIGHB_RAD(neighb_rad)
	, PREDICTION_DELAY(prediction_delay)
	, NN_K(nn_k)
	, NN_K_COORDS(nn_k_coords)
	, MULT_INT(mult_int)
	, SIGMA_X(sigma_x)
	, SIGMA_A(sigma_a)
	, SIGMA_XX(sigma_xx)
	, HMM_TP_WEIGHT(1.0)
	, SWR_SWITCH(buf->config_->getBool("kd.swr.switch", false))
	, SWR_SLOWDOWN_DELAY(buf->config_->getInt("kd.swr.slowdown.delay", 0)){

	const unsigned int tetrn = buf->tetr_info_->tetrodes_number;

	kdtrees_.resize(tetrn);
	ann_points_.resize(tetrn);
	total_spikes_.resize(tetrn);
	obs_spikes_.resize(tetrn);
	knn_cache_.resize(tetrn);
	spike_place_fields_.resize(tetrn);
	ann_points_int_.resize(tetrn);
	obs_mats_.resize(tetrn);
	coords_normalized_.resize(tetrn);
	missed_spikes_.resize(tetrn);

	kdtrees_coords_.resize(tetrn);

	fitting_jobs_.resize(tetrn, nullptr);
	fitting_jobs_running_.resize(tetrn, false);

	laxs_.resize(tetrn);
	pxs_.resize(tetrn, arma::mat(NBINS, NBINS, arma::fill::zeros));
	lxs_.resize(tetrn, arma::mat(NBINS, NBINS, arma::fill::zeros));

	for (int t = 0; t < tetrn; ++t) {
		ann_points_[t] = annAllocPts(MIN_SPIKES, DIM);
		spike_place_fields_[t].reserve(MIN_SPIKES);

		ann_points_int_[t] = new int*[MIN_SPIKES];
		for (int d = 0; d < MIN_SPIKES; ++d) {
			ann_points_int_[t][d] = new int[DIM];
		}

		// tmp
		obs_mats_[t] = arma::mat(MIN_SPIKES, 14);

		// bin / (x,y)
		coords_normalized_[t] = arma::Mat<int>(NBINS, 2, arma::fill::zeros);

		laxs_[t].resize(MIN_SPIKES, arma::mat(NBINS, NBINS, arma::fill::zeros));
	}

	pf_built_.resize(tetrn);

	pix_log_ = arma::mat(NBINS, NBINS, arma::fill::zeros);
	pix_ = arma::mat(NBINS, NBINS, arma::fill::zeros);

	pos_pred_ = arma::mat(NBINS, NBINS, arma::fill::zeros);

	if (LOAD){
		// load occupancy
		pix_.load(BASE_PATH + "pix.mat");
		pix_log_.load(BASE_PATH + "pix_log.mat");

		std::vector<std::thread*> load_threads;
		for (int t = 0; t < tetrn; ++t) {
//			load_threads.push_back(new std::thread(&KDClusteringProcessor::load_laxs_tetrode, this, t));
			load_laxs_tetrode(t);
		}
//		for (int t = 0; t < tetrn; ++t) {
//			load_threads[t]->join();
//		}
//		std::cout << "All laxs loaded \n";

		n_pf_built_ = tetrn;

		if (USE_PRIOR){
			pos_pred_ = pix_log_;
		}
	}

	tetr_spiked_ = std::vector<bool>(buffer->tetr_info_->tetrodes_number, false);
	// posterior position probabilities map
	// initialize with log of prior = pi(x)

	// TODO !!! priors
	reset_hmm();

	// STATS
	dist_swr_.open((std::string("dist_swr.txt")));
	dist_theta_.open((std::string("dist_theta.txt")));
	err_bay_.open((std::string("err_bay.txt")));
	err_hmm_.open((std::string("err_hmm.txt")));
	dec_coords_.open("dec_coords.txt");

	hmm_traj_.resize(NBINS * NBINS);

	std::string parpath = base_path + "params.txt";
	std::ofstream fparams(parpath);
	fparams << "SIGMA_X, SIGMA_A, SIGMA_XX, MULT_INT, SAMPLING_RATE, NN_K, NN_K_SPACE(obsolete), MIN_SPIKES, SAMPLING_RATE, SAMPLING_DELAY, NBINS, BIN_SIZE\n" <<
			sigma_x << " " << sigma_a << " " << sigma_xx<< " " << mult_int << " " << sampling_rate<< " "
			<< nn_k << " "<< nn_k_coords << " "
			<< MIN_SPIKES << " " << sampling_rate << " " << sampling_delay<< " " << NBINS << " " << BIN_SIZE << "\n";
	fparams.close();
	std::cout << "Running params written to " << parpath << "\n";
}

KDClusteringProcessor::~KDClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

const arma::mat& KDClusteringProcessor::GetPrediction() {
	return last_pred_probs_;
}

void KDClusteringProcessor::update_hmm_prediction() {
	// old hmm_prediction + transition (without evidence)
	arma::mat hmm_upd_(NBINS, NBINS, arma::fill::zeros);

	// TODO CONTROLLED reset, PARAMETRIZE
	if (!(buffer->last_preidction_window_end_ % HMM_RESET_RATE)){
		// DEBUG
		std::cout << "Reset HMM at " << buffer->last_preidction_window_end_  << "..\n";

		if (USE_PRIOR){
			hmm_prediction_ = pix_log_;
		}
		else{
			hmm_prediction_ = arma::mat(NBINS, NBINS, arma::fill::zeros);
		}
	}

	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			// find best (x, y) - with highest probability of (x,y)->(xb,yb)
			// implement hmm{t}(xb,yb) = max_{x,y \in neighb(xb,yb)}(hmm_{t-1}(x,y) * tp(x,y,xb,yb) * prob(xb, yb | a_{1..N}))

			float best_to_xb_yb = -1000.0f * 100000000.0f;
			int bestx, besty;

			for (int x = MAX(0, xb - HMM_NEIGHB_RAD); x <= MIN(xb + HMM_NEIGHB_RAD, NBINS - 1); ++x) {
				for (int y =  MAX(0, yb - HMM_NEIGHB_RAD); y <= MIN(yb + HMM_NEIGHB_RAD, NBINS - 1); ++y) {
					// TODO weight
					// split for DEBUG
					float prob_xy = hmm_prediction_(x, y);
					int shx = xb - x + HMM_NEIGHB_RAD;
					int shy = yb - y + HMM_NEIGHB_RAD;

					prob_xy += HMM_TP_WEIGHT * buffer->tps_[y * NBINS + x](shx, shy);
					if (prob_xy > best_to_xb_yb){
						best_to_xb_yb = prob_xy;
						bestx = x;
						besty = y;
					}
				}
			}

			hmm_upd_(xb, yb) = best_to_xb_yb;

			hmm_traj_[yb * NBINS + xb].push_back(besty * NBINS + bestx);
		}
	}

	// DEBUG - check that no window is skipped and the chain is broken
	if(!(hmm_traj_[0].size() % 2000)){
		std::cout << "hmm bias control: " << hmm_traj_[0].size() << " / " << (int)round(last_pred_pkg_id_ / (float)PRED_WIN) << "\n";
		// DEBUG
		hmm_prediction_.save(BASE_PATH + "hmm_pred_" + Utils::Converter::int2str(hmm_traj_[0].size()) + ".mat", arma::raw_ascii);
	}

	// renorm > (subtract min)
	// DEBUG
//	std::cout << "hmm before upd with evidence:" << hmm_upd_ << "\n\n";

	// add Bayesian pos likelihood from evidence
	hmm_prediction_ = hmm_upd_ + last_pred_probs_;

	// VISUALIZATION ADJUSTMENT:to avoid overflow
	hmm_prediction_ = hmm_prediction_ - hmm_prediction_.max();

	// DEBUG
//	hmm_prediction_.save(BASE_PATH + "hmm_pred_" + Utils::Converter::int2str(hmm_traj_[0].size()) + ".mat", arma::raw_ascii);

	// STATS - write error of Bayesian and HMM, compare to pos in the middle of the window
	int ind = (int)round((last_pred_pkg_id_ - PRED_WIN/2)/512.0);
	int corrx = buffer->positions_buf_[ind][0];
	int corry = (int)buffer->positions_buf_[ind][1];

//	if (corrx != 1023 && last_pred_pkg_id_ > 20 * 1000000){
		unsigned int x,y;
		last_pred_probs_.max(x, y);
		x = BIN_SIZE * (x + 0.5);
		y = BIN_SIZE * (y + 0.5);
		float eb = (corrx - x) * (corrx - x) + (corry - y) * (corry - y);
		err_bay_ << sqrt(eb) << "\n";
		dec_coords_ << x << " " << y << " " << corrx << " " << corry << "\n";
		dec_coords_.flush();
//	}

	// for consistency of comparison
	if (last_pred_pkg_id_ > 46 * 1000000){
		// STATS - dump best HMM trajectory by backtracking
		std::ofstream dec_hmm("dec_hmm.txt");
		int t = hmm_traj_[0].size() - 1;
		// best last x,y
		unsigned int x,y;
		hmm_prediction_.max(x, y);
		while (t >= 0){
			dec_hmm << BIN_SIZE * (x + 0.5) << " " << BIN_SIZE * (y + 0.5) << " ";
			corrx = buffer->positions_buf_[(int)((t * 2000 + PREDICTION_DELAY) / 512.0)][0];
			corry = buffer->positions_buf_[(int)((t * 2000 + PREDICTION_DELAY)/ 512.0)][1];
			dec_hmm << corrx << " " << corry << "\n";
			int b = hmm_traj_[y * NBINS + x][t];
			// TODO !!! fix
			// WORKAROUND - keep the previous position
			if (b < NBINS * NBINS){
				y = b / NBINS;
				x = b % NBINS;
			}
			else
			{
				std::cout << "Trajectory is screwed up at " << t << ", keep the previous position\n";
			}

			t--;
		}
		dec_hmm.flush();

		std::cout << "Exit after dumping the HMM prediction...\n";
		// TODO: dump in constructor / parametrized
		exit(0);
	}


	// DEBUG
//	std::cout << "hmm after upd with evidence:" << hmm_prediction_ << "\n\n";

	buffer->last_prediction_ = arma::exp(hmm_prediction_.t() / 200);
}

void KDClusteringProcessor::reset_hmm() {
	hmm_prediction_ = arma::mat(NBINS, NBINS, arma::fill::zeros);
	if (USE_PRIOR){
		hmm_prediction_ = pix_log_;
	}
}

void KDClusteringProcessor::process(){
	// TODO !!! check if buffer size affects prediction quality

	// need both speed and PCs
	while(buffer->spike_buf_pos_clust_ < MIN(buffer->spike_buf_pos_speed_, buffer->spike_buf_pos_unproc_)){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
		const unsigned int tetr = spike->tetrode_;

		// wait until place fields are stabilized
		if (spike->pkg_id_ < SAMPLING_DELAY && !LOAD){
			buffer->spike_buf_pos_clust_ ++;
			continue;
		}

		// DEBUG
		if (!delay_reached_reported){
			delay_reached_reported = true;
			std::cout << "Delay over (" << SAMPLING_DELAY << "). Start spike collection...\n";
		}

		if (spike->speed < SPEED_THOLD || spike->discarded_){
			buffer->spike_buf_pos_clust_ ++;
			continue;
		}

		if (!pf_built_[tetr]){
			// to start prediction only after all PFs are available
			last_pred_pkg_id_ = spike->pkg_id_;

			if (total_spikes_[tetr] >= MIN_SPIKES){
				// build the kd-tree and call kNN for all points, cache indices (unsigned short ??) in the array of pointers to spikes

				// start clustering if it is not running yet, otherwise - ignore
				if (!fitting_jobs_running_[tetr]){
//					build_lax_and_tree_separate(tetr);

					fitting_jobs_running_[tetr] = true;
					fitting_jobs_[tetr] = new std::thread(&KDClusteringProcessor::build_lax_and_tree_separate, this, tetr);
//
//					// !!! WORKAROUND due to thread-unsafety of ANN
//					fitting_jobs_[tetr]->join();
//					fitting_jobs_running_[tetr] = false;
				}
			}
			else{
				// sample every SAMLING_RATE spikes for KDE estimation
				if (missed_spikes_[tetr] < SAMPLING_RATE - 1){
					missed_spikes_[tetr] ++;
					buffer->spike_buf_pos_clust_ ++;
					continue;
				}
				missed_spikes_[tetr] = 0;

				obs_spikes_[tetr].push_back(spike);

				// copy features and coords to ann_points_int and obs_mats
				for (int pc=0; pc < 3; ++pc) {
					// TODO: tetrode channels
					for(int chan=0; chan < 4; ++chan){
						ann_points_[tetr][total_spikes_[tetr]][chan * 3 + pc] = spike->pc[chan][pc];

						// save integer with increased precision for integer KDE operations
						ann_points_int_[tetr][total_spikes_[tetr]][chan * 3 + pc] = (int)round(spike->pc[chan][pc] * MULT_INT);

						// set from the obs_mats after computing the coordinates normalizing factor
//						spike_coords_int_[tetr](total_spikes_[tetr], 0) = (int)round(spike->x * MULT_INT);
//						spike_coords_int_[tetr](total_spikes_[tetr], 1) = (int)round(spike->y * MULT_INT);

						// tmp: for stats
						obs_mats_[tetr](total_spikes_[tetr], chan * 3 + pc) = spike->pc[chan][pc];
					}
				}

				obs_mats_[tetr](total_spikes_[tetr], N_FEAT) = spike->x;
				obs_mats_[tetr](total_spikes_[tetr], N_FEAT + 1) = spike->y;

				total_spikes_[tetr] ++;
			}

			buffer->spike_buf_pos_clust_ ++;
		}
		else{
			// if pf_built but job is designated as running, then it is over and can be joined
			if (fitting_jobs_running_[tetr]){
				fitting_jobs_running_[tetr] = false;
				fitting_jobs_[tetr]->join();

				if (USE_PRIOR){
					pos_pred_ = pix_log_;
				}
			}

			// predict from spike in window

			// prediction only after having on all fields
			if (n_pf_built_ < buffer->tetr_info_->tetrodes_number){
				buffer->spike_buf_pos_clust_ ++;
				continue;
			}

			// prediction only after reaching delay (place field stability, cross-validation etc.)
			if (buffer->spike_buffer_[buffer->spike_buf_pos_clust_]->pkg_id_ < PREDICTION_DELAY){
				buffer->spike_buf_pos_clust_ ++;
				continue;
			} else if (!prediction_delay_reached_reported){
				std::cout << "Prediction delay over (" << PREDICTION_DELAY << ").\n";
				prediction_delay_reached_reported = true;

				last_pred_pkg_id_ = PREDICTION_DELAY;
				buffer->last_preidction_window_end_ = PREDICTION_DELAY;
			}

			// check if NEW swr was detected and has to switch to the SWR regime
			if (SWR_SWITCH){
				if (!swr_regime_ && !buffer->swrs_.empty() && buffer->swrs_.front()[0] != last_processed_swr_start_){
					//DEBUG
					std::cout << "Switch to SWR prediction regime due to SWR detected at " << buffer->swrs_.front()[0] << "\n";

					swr_regime_ = true;
					// TODO configurableize
					PRED_WIN = 300;

					last_pred_pkg_id_ = buffer->swrs_.front()[0];
					last_processed_swr_start_ = buffer->swrs_.front()[0];

					// rewind until the first spike in the SW
					while(spike->pkg_id_ > last_pred_pkg_id_){
						// TODO OOB control
						buffer->spike_buf_pos_clust_ --;
						spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
					}

					// reset HMM
					reset_hmm();
				}

				if (swr_regime_ && last_pred_pkg_id_ > buffer->swrs_.front()[2]){
					// DEBUG
					std::cout << "Switch to theta prediction regime due to end of SWR at " <<  buffer->swrs_.front()[2] << "\n";
					swr_regime_ = false;
					PRED_WIN = 2000;

					// reset HMM
					reset_hmm();
					buffer->swrs_.pop();
					swr_counter_ ++;
					swr_win_counter_ = 0;
				}
			}

			ANNpoint pnt = annAllocPt(DIM);
			double dist;
			int closest_ind;
			// at this points all tetrodes have pfs !
			// TODO don't need speed (for prediction), can take more spikes
			while(spike->pkg_id_ < last_pred_pkg_id_ + PRED_WIN && buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos_speed_){
				const unsigned int stetr = spike->tetrode_;
				tetr_spiked_[stetr] = true;

				// TODO: convert PC in spike to linear array
				for (int pc=0; pc < 3; ++pc) {
					// TODO: tetrode channels
					for(int chan=0; chan < 4; ++chan){
						pnt[chan * 3 + pc] = spike->pc[chan][pc];
					}
				}

				// PROFILE
//				time_t kds = clock();
				// 5 us for eps = 0.1, 20 ms - for eps = 10.0, prediction quality - ???
				// TODO : quantify dependence of prediction quality on the EPS
				kdtrees_[stetr]->annkSearch(pnt, 1, &closest_ind, &dist, NN_EPS);
//				std::cout << "kd time = " << clock() - kds << "\n";

				// add 'place field' of the spike with the closest wave shape
				pos_pred_ += laxs_[stetr][closest_ind];

				buffer->spike_buf_pos_clust_++;
				// spike may be without speed (the last one) - but it's not crucial
				spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
			}

			// if prediction is final and end of window has been reached (last spike is beyond the window)
			// 		or prediction will be finalized in subsequent iterations
			if(spike->pkg_id_ >= last_pred_pkg_id_ + PRED_WIN){

				// edges of the window
				const double DE_SEC = PRED_WIN / (float)buffer->SAMPLING_RATE;

				for (int t = 0; t < buffer->tetr_info_->tetrodes_number; ++t) {
					if (tetr_spiked_[t]){
						// TODO: depricated LX_WEIGHT, was introduced only for debugging purposes
						pos_pred_ -= DE_SEC  * lxs_[t];
					}
				}

				//DEBUG - slow down to see SWR prediction
				if (swr_regime_){
					std::cout << "prediction within SWR...\n";

					if (buffer->last_pkg_id > SWR_SLOWDOWN_DELAY){
						// usleep(1000 * 1500);
					}

				}

				last_pred_probs_ = pos_pred_;
				// to avoid OOR
				double minpred = arma::max(arma::max(last_pred_probs_));
				last_pred_probs_ -= minpred;

//				double minval = arma::min(arma::min(pos_pred_));
//				pos_pred_ = pos_pred_ - minval;
				pos_pred_ = arma::exp(pos_pred_ / 200);

				// updated in HMM
				buffer->last_prediction_ = pos_pred_.t();
				buffer->last_preidction_window_end_ = last_pred_pkg_id_ + PRED_WIN;

				if (swr_regime_){
					// DEBUG save prediction
					pos_pred_.save("../out/jc84_1910/" + std::string("swr_") + Utils::Converter::int2str(swr_counter_) + "_" + Utils::Converter::int2str(swr_win_counter_) + ".mat", arma::raw_ascii);
					swr_win_counter_ ++;
				}
				else{
					pos_pred_.save("../out/jc84_1910/" + std::string("learn_") + Utils::Converter::int2str(swr_win_counter_) + ".mat", arma::raw_ascii);
					swr_win_counter_ ++;
				}

				// DEBUG
//				if (!(last_pred_pkg_id_ % 200)){
//					pos_pred_.save(BASE_PATH + "tmp_pred_" + Utils::Converter::int2str(last_pred_pkg_id_) + ".mat", arma::raw_ascii);
//					std::cout << "save prediction...\n";
//				}

				last_pred_pkg_id_ += PRED_WIN;

				// re-init prediction variables
				tetr_spiked_ = std::vector<bool>(buffer->tetr_info_->tetrodes_number, false);

//				reset_hmm();

				if (USE_HMM)
					update_hmm_prediction();

				// DEBUG
				npred ++;
				if (!(npred % 2000)){
					std::cout << "Bayesian prediction window bias control: " << npred << " / " << (int)round(last_pred_pkg_id_ / (float)PRED_WIN) << "\n";
				}

				// TODO: extract
				pos_pred_ = USE_PRIOR ? (buffer->tetr_info_->tetrodes_number * pix_log_) : arma::mat(NBINS, NBINS, arma::fill::zeros);

				// return to display prediction etc...
				//		(don't need more spikes at this stage)
				return;
			}
			// marginal rate function at each tetrode
		}
	}
}

void KDClusteringProcessor::build_lax_and_tree_separate(const unsigned int tetr) {
	// dump required data and start process (due to non-thread-safety of ANN)
	// build tree and dump it along with points
	std::cout << "t " << tetr << ": build kd-tree for tetrode " << tetr << ", " << n_pf_built_ << " / " << buffer->tetr_info_->tetrodes_number << " finished... ";
	kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], DIM);
	std::cout << "done\nt " << tetr << ": cache " << NN_K << " nearest neighbours for each spike in tetrode " << tetr << " (in a separate thread)...\n";

	std::ofstream kdstream(BASE_PATH + Utils::Converter::int2str(tetr) + ".kdtree");
	kdtrees_[tetr]->Dump(ANNtrue, kdstream);
	kdstream.close();

	// dump obs_mat
	obs_mats_[tetr].save(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_obs.mat");

	// create pos_buf and dump (first count points),
	// TODO cut matrix later
	unsigned int npoints = 0;
	// TODO sparse sampling
	arma::Mat<int> pos_buf(2, buffer->pos_buf_pos_);
	for (int n = 0; n < buffer->pos_buf_pos_; ++n) {
		// if pos is unknown or speed is below the threshold - ignore
		if (buffer->positions_buf_[n][0] == 1023 || buffer->positions_buf_[n][5] < SPEED_THOLD){
			continue;
		}
		pos_buf(0, npoints) = buffer->positions_buf_[n][0];
		pos_buf(1, npoints) = buffer->positions_buf_[n][1];
		npoints++;
	}
	pos_buf = pos_buf.cols(0, npoints - 1);
	pos_buf.save(BASE_PATH  + "tmp_" + Utils::NUMBERS[tetr] + "_pos_buf.mat");

	/// buuild commandline to start kde_estimator
	std::ostringstream  os;
	os << "./kde_estimator " << tetr << " " << DIM << " " << NN_K << " " << NN_K_COORDS << " " << N_FEAT << " " <<
			MULT_INT << " " << BIN_SIZE << " " << NBINS << " " << MIN_SPIKES << " " <<
			SAMPLING_RATE << " " << buffer->SAMPLING_RATE << " " << buffer->last_pkg_id << " " << SAMPLING_DELAY << " " << NN_EPS
			<< " " << SIGMA_X << " " << SIGMA_A << " " << SIGMA_XX << " " << BASE_PATH;
	std::cout << "t " << tetr << ": Start external kde_estimator with command (tetrode, dim, nn_k, nn_k_coords, n_feat, mult_int,  bin_size, n_bins. min_spikes, sampling_rate, buffer_sampling_rate, last_pkg_id, sampling_delay, nn_eps, sigma_x, sigma_a, sigma_xx)\n\t" << os.str() << "\n";

//	if (tetr == 13)
	int retval = system(os.str().c_str());
	if (retval < 0){
		std::cout << "ERROR: impossible to start kde_estimator!\n";
	}
	else{
		std::cout << "t " << tetr << ": kde estimator exited with code " << retval << "\n";
	}

	pf_built_[tetr] = true;

	// TODO competitive
	n_pf_built_ ++;

	if (n_pf_built_ == buffer->tetr_info_->tetrodes_number){
		std::cout << "KDE at all tetrodes done\n";
	}
}

void KDClusteringProcessor::JoinKDETasks(){
    for(int t=0; t < buffer->tetr_info_->tetrodes_number; ++t){
        if(fitting_jobs_running_[t])
            fitting_jobs_[t]->join();
    }
    std::cout << "All KDE jobs joined...\n";
}
