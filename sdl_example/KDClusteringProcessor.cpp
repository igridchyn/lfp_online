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

KDClusteringProcessor::KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes,
		const std::string base_path, PlaceFieldProcessor* pfProc,
		const unsigned int sampling_delay, const bool save, const bool load, const bool use_prior,
		const unsigned int sampling_rate, const float speed_thold, const bool use_marginal, const float eps,
		const bool use_hmm, const unsigned int nbins, const unsigned int bin_size, const int neighb_rad,
		const unsigned int prediction_delay, const unsigned int nn_k, const unsigned int nn_k_coords,
		const unsigned int mult_int, const float lx_weight, const float hmm_tp_weight,
		const double sigma_x, const double sigma_a)
	: LFPProcessor(buf)
	, MIN_SPIKES(num_spikes)
	//, BASE_PATH("/hd1/data/bindata/jc103/jc84/jc84-1910-0116/pf_ws/pf_"){
	, BASE_PATH(base_path)
	, pfProc_(pfProc)
	, SAMPLING_DELAY(sampling_delay)
	, SAVE(save)
	, LOAD(load)
	, USE_PRIOR(use_prior)
	, SAMPLING_RATE(sampling_rate)
	, SPEED_THOLD(speed_thold)
	, USE_MARGINAL(use_marginal)
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
	, LX_WEIGHT(lx_weight)
	, HMM_TP_WEIGHT(hmm_tp_weight){
	// TODO Auto-generated constructor stub

	const unsigned int tetrn = buf->tetr_info_->tetrodes_number;
	spike_buf_pos_pred_ = buffer->SPIKE_BUF_HEAD_LEN;

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

	fitting_jobs_.resize(tetrn, NULL);
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
}

KDClusteringProcessor::~KDClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

// bulid p(a,x) with a of a given spike and (x,y)  coordinates of centers of time bins (normalized)
void KDClusteringProcessor::build_pax_(const unsigned int tetr, const unsigned int spikei, const arma::mat& occupancy, const arma::Mat<int>& spike_coords_int) {
	arma::mat pf(NBINS, NBINS, arma::fill::zeros);

	const double occ_sum = arma::sum(arma::sum(occupancy));

	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			// order of >= 30
			double kern_sum = 0;

			unsigned int nspikes = 0;
			// compute KDE (proportional to log-probability) over cached nearest neighbours
			// TODO ? exclude self from neighbours list ?
			for (int ni = 1; ni < NN_K; ++ni) {
				Spike *spike = obs_spikes_[tetr][knn_cache_[tetr][spikei][ni]];
				if (spike->x == 1023){
					continue;
				}

				long long logprob = kern_H_ax_(spikei, knn_cache_[tetr][spikei][ni], tetr,
						coords_normalized_[tetr](xb, 0), coords_normalized_[tetr](yb, 1),
						spike_coords_int);

				// DEBUG
				if (logprob > 0){
//					std::cout << "alarm : positive log-prob\n";
//					logprob = kern_(spikei, knn_cache_[tetr][spikei][ni], tetr, coords_normalized_[tetr](xb, 0), coords_normalized_[tetr](yb, 1));
					continue;
				}

				nspikes++;
				kern_sum += exp((double)logprob / (MULT_INT * MULT_INT));
			}

			// scaled by MULT_INT ^ 2
//			pf(xb, yb) = (double)kern_sum / (MULT_INT * MULT_INT) / nspikes;
			if (occupancy(xb, yb)/occ_sum >= 0.001){
				pf(xb, yb) = log(kern_sum / nspikes / occupancy(xb, yb));
			}
		}
	}

	double occ_min = pf.min();
	// set min at low occupancy
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			if (occupancy(xb, yb)/occ_sum < 0.001){
				pf(xb, yb) = occ_min;
			}
		}
	}

	spike_place_fields_[tetr][spikei] = pf;

	if (SAVE){
		std::string save_path = BASE_PATH + Utils::NUMBERS[tetr] + "_" + Utils::Converter::int2str((int)spikei) + ".mat";
		pf.save(save_path, arma::raw_ascii);
//		std::cout << save_path << "\n";
	}

	laxs_[tetr][spikei] = pf;
}

// x/y instead of coords of the second spike
long long inline KDClusteringProcessor::kern_H_ax_(const unsigned int spikei1, const unsigned int spikei2,
		const unsigned int tetr, const int& x, const int& y,
		const arma::Mat<int>& spike_coords_int) {
	// TODO: implement efficiently (integer with high precision)
	// TODO: check for the overlow (MULT order X DIM X MAX(coord order / feature sorder) X squared = (10 + 1 + 2) * 2 = 26 < 32 bit )

	long long sum = 0;
//	sum += (obs_spikes_[tetr][spikei1]->x - obs_spikes_[tetr][spikei2]->x) / X_STD;
//	sum += (obs_spikes_[tetr][spikei1]->y - obs_spikes_[tetr][spikei2]->y) / Y_STD;

	int *pcoord1 = ann_points_int_[tetr][spikei1], *pcoord2 = ann_points_int_[tetr][spikei2];
	for (int d = 0; d < DIM; ++d, ++pcoord1, ++pcoord2) {
		int coord1 = *pcoord1, coord2 = *pcoord2;
		sum += (coord1 - coord2) * (coord1 - coord2);
//		sum += (*pcoord1 - *pcoord2) ^ 2;
	}

	// coords are already normalized to have the same variance as features (average)
	// X coordinate

	int neighbx = spike_coords_int(spikei2, 0);
	int xdiff = (neighbx - x);
	sum += xdiff * xdiff;
	// Y coordinate
	int neighby = spike_coords_int(spikei2, 1);
	int ydiff = (neighby - y);
	sum += ydiff * ydiff;

//	ANNcoord *pcoord1 = ann_points_[tetr][spikei1], *pcoord2 = ann_points_[tetr][spikei2];

	// unroll loop - 20% faster
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;
//	sum += (*(pcoord1++) - *(pcoord2++)) ^ 2;

	// order : ~ 10^9 for 12 features scaled by 2^10 = 10^3
	return - sum / 2;
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
					// TODO !!! parametrize
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
	if (last_pred_pkg_id_ > 55 * 1000000){
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

	buffer->last_prediction_ = arma::exp(hmm_prediction_.t() / 1000);
}

void KDClusteringProcessor::reset_hmm() {
	hmm_prediction_ = arma::mat(NBINS, NBINS, arma::fill::zeros);
	if (USE_PRIOR){
		hmm_prediction_ = pix_log_;
	}
}

void KDClusteringProcessor::process(){
	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos_speed_){
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

		if (spike->speed < SPEED_THOLD){
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
				if (missed_spikes_[tetr] < SAMPLING_RATE){
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

			// edges of the window
			const double DE_SEC = 100 / 1000.0;

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
					PRED_WIN = 400;

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
				// 0.5 ms for eps = 0.1, 0.02 ms - for eps = 10.0, prediction quality - ???
				// TODO : quantify dependence of prediction quality on the EPS
				kdtrees_[stetr]->annkSearch(pnt, 1, &closest_ind, &dist, NN_EPS);
//				std::cout << "kd time = " << clock() - kds << "\n";

				// STATS - distances to the nearest neighbour - to compare distributions in SWR and theta
//				if (swr_regime_){
//					dist_swr_ << dist << "\n";
//				}
//				else{
//					dist_theta_ << dist << "\n";
//				}

				// add 'place field' of the spike with the closest wave shape
				pos_pred_ += laxs_[stetr][closest_ind];

				buffer->spike_buf_pos_clust_++;
				// spike may be without speed (the last one) - but it's not crucial
				spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
			}

			// if prediction is final and end of window has been reached (last spike is beyond the window)
			// 		or prediction will be finalized in subsequent iterations
			if(spike->pkg_id_ >= last_pred_pkg_id_ + PRED_WIN){
				if (USE_MARGINAL){
					for (int t = 0; t < buffer->tetr_info_->tetrodes_number; ++t) {
						if (tetr_spiked_[t]){
							pos_pred_ -= LX_WEIGHT * DE_SEC  * lxs_[t];
						}
					}
				}

				//DEBUG - slow down to see SWR prediction
				if (swr_regime_){
					std::cout << "prediction within SWR...\n";

					if (buffer->last_pkg_id > SWR_SLOWDOWN_DELAY){
						usleep(1000 * 1500);
					}
				}

				last_pred_probs_ = pos_pred_;
				// to avoid OOR
				double minpred = arma::max(arma::max(last_pred_probs_));
				last_pred_probs_ -= minpred;

//				double minval = arma::min(arma::min(pos_pred_));
//				pos_pred_ = pos_pred_ - minval;
				pos_pred_ = arma::exp(pos_pred_ / 300);

				// updated in HMM
				buffer->last_prediction_ = pos_pred_.t();
				buffer->last_preidction_window_end_ = last_pred_pkg_id_ + PRED_WIN;

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
			SAMPLING_RATE << " " << buffer->SAMPLING_RATE << " " << buffer->last_pkg_id << " " << SAMPLING_DELAY << " " << NN_EPS << " " << SIGMA_X << " " << SIGMA_A << " " << BASE_PATH;
	std::cout << "t " << tetr << ": Start external kde_estimator with command (tetrode, dim, nn_k, nn_k_coords, n_feat, mult_int,  bin_size, n_bins. min_spikes, sampling_rate, buffer_sampling_rate, last_pkg_id, sampling_delay, nn_eps, sigma_x, sigma_a,)\n\t" << os.str() << "\n";

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

void KDClusteringProcessor::build_lax_and_tree(const unsigned int tetr) {
	std::cout << "build kd-tree for tetrode " << tetr << ", " << n_pf_built_ << " / " << buffer->tetr_info_->tetrodes_number << " finished... ";
	kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], DIM);
	std::cout << "done\n Cache " << NN_K << " nearest neighbours for each spike...\n";

	ANNpointArray ann_points_coords = annAllocPts(MIN_SPIKES, 2);
	arma::Mat<int> spike_coords_int(MIN_SPIKES, 2, arma::fill::zeros);

	if (SAVE){
		std::ofstream kdtree_stream(BASE_PATH + Utils::NUMBERS[tetr] + "_kdtree.mat");
		kdtrees_[tetr]->Dump(ANNtrue, kdtree_stream);
		kdtree_stream.close();
	}

	// Workaround for first-time saving
	//					pf_built_[tetr] = true;
	//					buffer->spike_buf_pos_clust_ ++;
	//					continue;

	// NORMALIZE STDS
	// TODO: !!! KDE / kd-tree search should be performed with the same std normalization !!!
	// current: don't normalize feature covariances (as clustering is done in this way) but normalize x/y std to have the average feature std
	std::vector<float> stds;
	float avg_feat_std = .0f;
	for (int f = 0; f < N_FEAT; ++f) {
		float stdf = arma::stddev(obs_mats_[tetr].col(f));
		std::cout << "std of feature " << f << " = " << stdf << "\n";
		stds.push_back(stdf);
		avg_feat_std += stdf;
	}
	avg_feat_std /= N_FEAT;
	float stdx = arma::stddev(obs_mats_[tetr].col(N_FEAT));
	float stdy = arma::stddev(obs_mats_[tetr].col(N_FEAT + 1));
	std::cout << "std of x  = " << stdx << "\n";
	std::cout << "std of y  = " << stdy << "\n";
	// normalize coords to have the average feature std
	for (int s = 0; s < total_spikes_[tetr]; ++s) {
		// ... loss of precision 1) from rounding to int; 2) by dividing int on float
		spike_coords_int(s, 0) = (int)(obs_mats_[tetr](s, N_FEAT) * avg_feat_std * MULT_INT / stdx);  //= stdx / avg_feat_std;
		spike_coords_int(s, 1) = (int)(obs_mats_[tetr](s, N_FEAT + 1) * avg_feat_std * MULT_INT / stdy);  //= stdy / avg_feat_std;

		// points to build coords 2d-tree, raw x and y coords
		ann_points_coords[s][0] = obs_mats_[tetr](s, N_FEAT);
		ann_points_coords[s][1] = obs_mats_[tetr](s, N_FEAT + 1);
	}

	// build 2d-tree for coords
	kdtrees_coords_[tetr] = new ANNkd_tree(ann_points_coords, total_spikes_[tetr], 2);
	// look for nearest neighbours of each bin center and compute p(x) - spike probability
	ANNidx *nnIdx_coord = new ANNidx[NN_K_COORDS];
	ANNdist *dists_coord = new ANNdist[NN_K_COORDS];
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			ANNpoint pnt = annAllocPt(2);
			double xc = BIN_SIZE * (0.5 + xb);
			double yc = BIN_SIZE * (0.5 + yb);
			pnt[0] = xc;
			pnt[1] = yc;
			kdtrees_coords_[tetr]->annkSearch(pnt, NN_K_COORDS, nnIdx_coord, dists_coord, NN_EPS);

			// compute KDE from neighbours
			double kde_sum = 0;
			unsigned int npoints = 0;
			for (int n = 0; n < NN_K_COORDS; ++n) {
				if (ann_points_coords[nnIdx_coord[n]][0] == 1023){
					continue;
				}
				npoints ++;

				double sum = 0;
				double xdiff = (xc - ann_points_coords[nnIdx_coord[n]][0]);
				sum += xdiff * xdiff / stdx;
				double ydiff = (yc - ann_points_coords[nnIdx_coord[n]][1]);
				sum += ydiff * ydiff / stdy;

				kde_sum += exp(- sum / 2);
			}

			kde_sum /= npoints;

			pxs_[tetr](xb, yb) = kde_sum;
		}
	}
	if (SAVE){
		pxs_[tetr].save(BASE_PATH + Utils::NUMBERS[tetr] + "_px.mat", arma::raw_ascii);
	}

	// compute occupancy KDE - pi(x) from tracking position sampling
	// TODO 2d-tree ? how many neighbours needed ?
	// overall tetrode average firing rate
	double mu = MIN_SPIKES * SAMPLING_RATE * buffer->SAMPLING_RATE / buffer->last_pkg_id;
	std::cout << "Average firing rate on tetrode: " << mu << "\n";
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			double xc = BIN_SIZE * (0.5 + xb);
			double yc = BIN_SIZE * (0.5 + yb);

			double kde_sum = 0;
			unsigned int npoints = 0;
			for (int n = 0; n < buffer->pos_buf_pos_; ++n) {
				if (buffer->positions_buf_[n][0] == 1023){
					continue;
				}
				npoints++;

				double sum = 0;

				double xdiff = xc - buffer->positions_buf_[n][0];
				double ydiff = yc - buffer->positions_buf_[n][1];

				sum += xdiff * xdiff / stdx;
				sum += ydiff * ydiff / stdy;

				kde_sum += exp(- sum / 2);
			}

			kde_sum /= npoints;

			pix_log_(xb, yb) = log(kde_sum);
			pix_(xb, yb) = kde_sum;
		}
	}

	// compute generalized rate function lambda(x)
	double pisum = arma::sum(arma::sum(pix_));
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			// absolute value of this function matter, but constant near p(x) and pi(x) is the same (as the same kernel K_H_x is used)
			// TODO !!! depends on number of bins !!! - parametrize
			if (pix_(xb, yb) > 0.00005 * pisum){
				lxs_[tetr](xb, yb) = mu * pxs_[tetr](xb, yb) / pix_(xb, yb);
			}
		}
	}

	if (SAVE){
		pix_log_.save(BASE_PATH + "pix_log.mat", arma::raw_ascii);
		pix_.save(BASE_PATH + "pix.mat", arma::raw_ascii);
		lxs_[tetr].save(BASE_PATH + Utils::NUMBERS[tetr] + "_lx.mat", arma::raw_ascii);
	}

	// pre-compute matrix of normalized bin centers
	// ASSUMING xbin size == ybin size
	for (int xb = 0; xb < NBINS; ++xb) {
		coords_normalized_[tetr](xb, 0) = (int)((float)BIN_SIZE * (0.5 + xb) * avg_feat_std * MULT_INT / stdx);
		coords_normalized_[tetr](xb, 1) = (int)((float)BIN_SIZE * (0.5 + xb) * avg_feat_std * MULT_INT / stdy);
	}

	// cache k nearest neighbours for each spike (for KDE computation)
	ANNdist *dists = new ANNdist[NN_K];
	// call kNN for each point
	for (int p = 0; p < total_spikes_[tetr]; ++p) {
		ANNidx *nnIdx = new ANNidx[NN_K];

		kdtrees_[tetr]->annkSearch(ann_points_[tetr][p], NN_K, nnIdx, dists, NN_EPS);
		// TODO: cast/copy to unsigned short array?
		knn_cache_[tetr].push_back(nnIdx);

		// DEBUG
		//					Utils::Output::printIntArray(nnIdx, 10);
	}
	delete dists;
	std::cout << "done\n";

	// compute p(a_i, x) for all spikes (as KDE of nearest neighbours neighbours)
	time_t start = clock();
	//const arma::mat& occupancy = pfProc_->GetSmoothedOccupancy();
	for (int p = 0; p < total_spikes_[tetr]; ++p) {
		// DEBUG
		if (!(p % 5000)){
			std::cout.precision(2);
			std::cout << p << " place fields built, last 5000 in " << (clock() - start)/ (float)CLOCKS_PER_SEC << " sec....\n";
			start = clock();

			// for profiling
			//					if (p > 2500)
			//						exit(0);
		}

		build_pax_(tetr, p, pix_, spike_coords_int);
	}

	pf_built_[tetr] = true;
	n_pf_built_ ++;

	// if save - concatenate all matrices laxs_[tetr] and save (along with individual, for fast visualization)
	if (SAVE){
		arma::mat laxs_tetr_(NBINS, NBINS * MIN_SPIKES);
		for (int s = 0; s < MIN_SPIKES; ++s) {
			laxs_tetr_.cols(s*NBINS, (s + 1) * NBINS - 1) = laxs_[tetr][s];
		}
		laxs_tetr_.save(BASE_PATH + Utils::NUMBERS[tetr] + "_tetr.mat");
	}

	annDeallocPts(ann_points_coords);
}

void KDClusteringProcessor::JoinKDETasks(){
    for(int t=0; t < buffer->tetr_info_->tetrodes_number; ++t){
        if(fitting_jobs_running_[t])
            fitting_jobs_[t]->join();
    }
    std::cout << "All KDE jobs joined...\n";
}
