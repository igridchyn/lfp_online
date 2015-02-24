/*
 * KDClusteringProcessor.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#include "KDClusteringProcessor.h"

#include <fstream>

void KDClusteringProcessor::load_laxs_tetrode(unsigned int t){
	buffer->Log("Load probability estimations for tetrode ", t);

	// load l(a,x) for all spikes of the tetrode
	//			for (int i = 0; i < MIN_SPIKES; ++i) {
	//				laxs_[t][i].load(BASE_PATH + Utils::NUMBERS[t] + "_" + Utils::Converter::int2str(i) + ".mat");
	//			}

	// 	beware of inexation changes
	std::ifstream kdtree_stream(BASE_PATH + Utils::NUMBERS[t] + ".kdtree.reduced");
	kdtrees_[t] = new ANNkd_tree(kdtree_stream);
	kdtree_stream.close();

	int NUSED = kdtrees_[t]->nPoints();
	laxs_[t].reserve(NUSED);

	// load binary combined matrix and extract individual l(a,x)
	arma::mat laxs_tetr_(NBINSX, NBINSY * NUSED);
	laxs_tetr_.load(BASE_PATH + Utils::NUMBERS[t] + "_tetr.mat");
	for (int s = 0; s < NUSED; ++s) {
		laxs_[t].push_back(laxs_tetr_.cols(s*NBINSY, (s + 1) * NBINSY - 1));

		if (!(s % 1000))
			laxs_[t][laxs_[t].size() - 1].save(BASE_PATH + Utils::NUMBERS[t] + "_" + Utils::Converter::int2str(s) + ".tmp", arma::raw_ascii);
	}
	//laxs_tetr_.save(BASE_PATH + Utils::NUMBERS[t] + "_tetr.mat");

	// load marginal rate function
	lxs_[t].load(BASE_PATH + Utils::NUMBERS[t] + "_lx.mat");
	double maxval = arma::max(arma::max(lxs_[t]));
	for (unsigned int xb = 0; xb < NBINSX; ++xb) {
		for (unsigned int yb = 0; yb < NBINSY; ++yb) {
			if (lxs_[t](xb, yb) == 0){
				lxs_[t](xb, yb) = maxval;
			}
		}
	}

	pxs_[t].load(BASE_PATH + Utils::NUMBERS[t] + "_px.mat");

	pf_built_[t] = true;
}

KDClusteringProcessor::KDClusteringProcessor(LFPBuffer* buf, const unsigned int& processor_number)
	: LFPProcessor(buf,processor_number),
			MIN_SPIKES(getInt("kd.min.spikes")),
			BASE_PATH(getOutPath("kd.path.base")),
			SAMPLING_DELAY(getInt("kd.sampling.delay")),
			SAVE(getBool("kd.save")),
			LOAD(!getBool("kd.save")),
			USE_PRIOR(getBool("kd.use.prior")),
			SAMPLING_RATE(getInt("kd.sampling.rate")),
			SPEED_THOLD(getFloat("kd.speed.thold")),
			NN_EPS(getFloat("kd.nn.eps")),
			USE_HMM(getBool("kd.use.hmm")),
			NBINSX(getInt("nbinsx")),
			NBINSY(getInt("nbinsy")),
			BIN_SIZE(getFloat("bin.size")),
			HMM_NEIGHB_RAD(getInt("kd.hmm.neighb.rad")),
			PREDICTION_DELAY(getInt("kd.prediction.delay")),
			NN_K(getInt("kd.nn.k")),
			NN_K_COORDS(getInt("kd.nn.k.space")),
			MULT_INT(getInt("kd.mult.int")),
			SIGMA_X(getFloat("kd.sigma.x")),
			SIGMA_A(getFloat("kd.sigma.a")),
			SIGMA_XX(getFloat("kd.sigma.xx")),
			SWR_SWITCH(buf->config_->getBool("kd.swr.switch", false)),
			SWR_SLOWDOWN_DELAY(buf->config_->getInt("kd.swr.slowdown.delay", 0)),
			SWR_SLOWDOWN_DURATION(buf->config_->getInt("kd.swr.slowdown.duration", 1500)),
			SWR_PRED_WIN(buf->config_->getInt("kd.swr.pred.win", 400)),
			DUMP_DELAY(buf->config_->getInt("kd.dump.delay", 46000000)),
			DUMP_END(buf->config_->getInt("kd.dump.end", 1000000000)),
			HMM_RESET_RATE(buf->config_->getInt("kd.hmm.reset.rate", 60000000)),
			use_intervals_(buf->config_->getBool("kd.use.intervals", false)),
			spike_buf_pos_clust_(buf->spike_buf_pos_clusts_[processor_number]),
			THETA_PRED_WIN(buf->config_->getInt("kd.pred.win", 2000)),
			SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD(buf->config_->getFloat("kd.spike.graph.cover.distance.threshold", 0)),
			SPIKE_GRAPH_COVER_NNEIGHB(buf->config_->getInt("kd.spike.graph.cover.nneighb", 1)),
			POS_SAMPLING_RATE(buf->config_->getFloat("pos.sampling.rate", 512.0)),
			FR_ESTIMATE_DELAY(buf->config_->getFloat("kd.frest.delay", 1000000)),
			DUMP_SPEED_THOLD(buf->config_->getFloat("kd.dump.speed.thold", .0)),
			WAIT_FOR_SPEED_EST(getBool("kd.wait.speed")),
			RUN_KDE_ON_MIN_COLLECTED(getBool("kd.run.on.min")),
            kde_path_(buf->config_->getString("kd.path", "./kde_estimator")){

	PRED_WIN = THETA_PRED_WIN;

	// load proper tetrode info
	if (buf->alt_tetr_infos_.size() < processor_number_ + 1){
		tetr_info_ = buf->alt_tetr_infos_[0];
		Log("WARNING: using default tetrode info for processor number", (int)processor_number_);
	}
	else{
		tetr_info_ = buf->alt_tetr_infos_[processor_number_];
	}

	const unsigned int tetrn = tetr_info_->tetrodes_number();

	kdtrees_.resize(tetrn);
	ann_points_.resize(tetrn);
	total_spikes_.resize(tetrn);
	obs_spikes_.resize(tetrn);
	knn_cache_.resize(tetrn);
	spike_place_fields_.resize(tetrn);
	ann_points_int_.resize(tetrn);
	obs_mats_.resize(tetrn);
	missed_spikes_.resize(tetrn);

	kdtrees_coords_.resize(tetrn);

	fitting_jobs_.resize(tetrn, nullptr);
	fitting_jobs_running_.resize(tetrn, false);

	laxs_.resize(tetrn);
	pxs_.resize(tetrn, arma::mat(NBINSX, NBINSY, arma::fill::zeros));
	lxs_.resize(tetrn, arma::mat(NBINSX, NBINSY, arma::fill::zeros));

	for (unsigned int t = 0; t < tetrn; ++t) {
		const unsigned int dim = buf->feature_space_dims_[t];

		ann_points_[t] = annAllocPts(MIN_SPIKES * 2, dim);
		spike_place_fields_[t].reserve(MIN_SPIKES * 2);

		ann_points_int_[t] = new int*[MIN_SPIKES * 2];
		for (unsigned int d = 0; d < MIN_SPIKES * 2; ++d) {
			ann_points_int_[t][d] = new int[dim];
		}

		// tmp
		obs_mats_[t] = arma::mat(MIN_SPIKES * 2, buffer->feature_space_dims_[t] + 2);
	}

	pf_built_.resize(tetrn);

	pix_log_ = arma::mat(NBINSX, NBINSY, arma::fill::zeros);
	pix_ = arma::mat(NBINSX, NBINSY, arma::fill::zeros);

	pos_pred_ = arma::mat(NBINSX, NBINSY, arma::fill::zeros);

	if (LOAD){
		// load occupancy
		pix_.load(BASE_PATH + "pix.mat");
		pix_log_.load(BASE_PATH + "pix_log.mat");

		std::vector<std::thread*> load_threads;
		for (unsigned int t = 0; t < tetrn; ++t) {
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

	tetr_spiked_ = std::vector<bool>(tetr_info_->tetrodes_number(), false);
	// posterior position probabilities map
	// initialize with log of prior = pi(x)

	// TODO !!! priors
	reset_hmm();

	hmm_traj_.resize(NBINSX * NBINSY);

	// save params only for model building scenario
	if (SAVE){
		std::string parpath = BASE_PATH + "params.txt";
		std::ofstream fparams(parpath);
		fparams << "SIGMA_X, SIGMA_A, SIGMA_XX, MULT_INT, SAMPLING_RATE, NN_K, NN_K_SPACE(obsolete), MIN_SPIKES, SAMPLING_RATE, SAMPLING_DELAY, NBINSX, NBINSY, BIN_SIZE\n" <<
				SIGMA_X << " " << SIGMA_A << " " << SIGMA_XX<< " " << MULT_INT << " " << SAMPLING_RATE<< " "
				<< NN_K << " "<< NN_K_COORDS << " "
				<< MIN_SPIKES << " " << SAMPLING_RATE << " " << SAMPLING_DELAY<< " " << NBINSX << " " << NBINSY << " " << BIN_SIZE << "\n";
		fparams.close();
		buffer->Log(std::string("Running params written to") + parpath);

		// save all params together with the model directory
		std::string all_params = buffer->config_->getAllParamsText();
		std::ofstream model_config(BASE_PATH + "params.conf");
		model_config << all_params;
		model_config.close();
	}

	if (use_intervals_){
		std::string intpath = buf->config_->getString("kd.intervals.path");
		std::ifstream intfile(intpath);
		while (!intfile.eof()){
			int ints = 0, inte = 0;
			intfile >> ints >> inte;
			interval_starts_.push_back(ints);
			interval_ends_.push_back(inte);
		}
		intfile.close();
	}

	buffer->last_predictions_.resize(processor_number_ + 1);

	dec_bayesian_.open("dec_bay.txt");
	window_spike_counts_.open("../out/window_spike_counts.txt");
}

KDClusteringProcessor::~KDClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

const arma::mat& KDClusteringProcessor::GetPrediction() {
	return last_pred_probs_;
}

void KDClusteringProcessor::update_hmm_prediction() {
	// old hmm_prediction + transition (without evidence)
	arma::mat hmm_upd_(NBINSX, NBINSY, arma::fill::zeros);

	// TODO CONTROLLED reset, PARAMETRIZE
	unsigned int prevmaxx = 0, prevmaxy = 0;

	bool reset = buffer->last_preidction_window_ends_[processor_number_] - last_hmm_reset_ > HMM_RESET_RATE;
	if (reset){
		// DEBUG
		buffer->Log("Reset HMM at ", (int)buffer->last_preidction_window_ends_[processor_number_]);

		hmm_prediction_.max(prevmaxx, prevmaxy);

		if (USE_PRIOR){
			hmm_prediction_ = pix_log_;
		}
		else{
			hmm_prediction_ = arma::mat(NBINSX, NBINSY, arma::fill::zeros);
		}

		last_hmm_reset_ = buffer->last_preidction_window_ends_[processor_number_];
	}

	for (unsigned int xb = 0; xb < NBINSX; ++xb) {
		for (unsigned int yb = 0; yb < NBINSY; ++yb) {
			// find best (x, y) - with highest probability of (x,y)->(xb,yb)
			// implement hmm{t}(xb,yb) = max_{x,y \in neighb(xb,yb)}(hmm_{t-1}(x,y) * tp(x,y,xb,yb) * prob(xb, yb | a_{1..N}))

			float best_to_xb_yb = -1000.0f * 100000000.0f;
			int bestx, besty;

			for (unsigned int x = MAX(0, xb - HMM_NEIGHB_RAD); x <= MIN(xb + HMM_NEIGHB_RAD, NBINSX - 1); ++x) {
				for (unsigned int y =  MAX(0, yb - HMM_NEIGHB_RAD); y <= MIN(yb + HMM_NEIGHB_RAD, NBINSY - 1); ++y) {
					// TODO weight
					// split for DEBUG
					float prob_xy = hmm_prediction_(x, y);
					int shx = xb - x + HMM_NEIGHB_RAD;
					int shy = yb - y + HMM_NEIGHB_RAD;

					prob_xy += buffer->tps_[y * NBINSX + x](shx, shy);
					if (prob_xy > best_to_xb_yb){
						best_to_xb_yb = prob_xy;
						bestx = x;
						besty = y;
					}
				}
			}

			hmm_upd_(xb, yb) = best_to_xb_yb;

			if (reset){
				// all leading to the best before the reset
				hmm_traj_[yb * NBINSX + xb].push_back(prevmaxy * NBINSX + prevmaxx);
			}
			else{
				hmm_traj_[yb * NBINSX + xb].push_back(besty * NBINSX + bestx);
			}
		}
	}

	// DEBUG - check that no window is skipped and the chain is broken
	if(!(hmm_traj_[0].size() % 2000)){
		buffer->log_string_stream_ << "hmm bias control: " << hmm_traj_[0].size() << " / " << (int)round((last_pred_pkg_id_ - PREDICTION_DELAY) / (float)PRED_WIN) << "\n";
		buffer->Log();
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
	int ind = (int)round((last_pred_pkg_id_ - PRED_WIN/2)/(float)POS_SAMPLING_RATE);
	float corrx = buffer->positions_buf_[ind].x_pos();
	float corry = buffer->positions_buf_[ind].y_pos();

	// for consistency of comparison
	if (last_pred_pkg_id_ > DUMP_DELAY){
		// STATS - dump best HMM trajectory by backtracking
		std::ofstream dec_hmm(std::string("dec_hmm_") + Utils::NUMBERS[processor_number_] +".txt");
		int t = hmm_traj_[0].size() - 1;
		// best last x,y
		unsigned int x,y;
		hmm_prediction_.max(x, y);
		while (t >= 0){
			dec_hmm << BIN_SIZE * (x + 0.5) << " " << BIN_SIZE * (y + 0.5) << " ";
			int posind = (int)((t * PRED_WIN + PREDICTION_DELAY) / (float)POS_SAMPLING_RATE);
			corrx = buffer->positions_buf_[posind].x_pos();
			corry = buffer->positions_buf_[posind].y_pos();
			dec_hmm << corrx << " " << corry << "\n";
			unsigned int b = hmm_traj_[y * NBINSX + x][t];
			// TODO !!! fix
			// WORKAROUND - keep the previous position
			if (b < NBINSX * NBINSY){
				y = b / NBINSX;
				x = b % NBINSX;
			}
			else
			{
				buffer->log_string_stream_ << "Trajectory is screwed up at " << t << ", keep the previous position\n";
				buffer->Log();
			}

			t--;
		}
		dec_hmm.flush();

		buffer->Log("Exit after dumping the HMM prediction :", (int)DUMP_DELAY);
		exit(0);
	}


	// DEBUG
//	std::cout << "hmm after upd with evidence:" << hmm_prediction_ << "\n\n";

	buffer->last_predictions_[processor_number_] = arma::exp(hmm_prediction_.t() / 200);
}

void KDClusteringProcessor::reset_hmm() {
	hmm_prediction_ = arma::mat(NBINSX, NBINSY, arma::fill::zeros);
	if (USE_PRIOR){
		hmm_prediction_ = pix_log_;
	}
}

void KDClusteringProcessor::process(){
	// TODO make callback for pipeline data over event
	if (buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER){
		for (size_t tetr=0; tetr < tetr_info_->tetrodes_number(); ++tetr){
			if (!pf_built_[tetr] && !fitting_jobs_running_[tetr]){
				buffer->log_string_stream_ << "t " << tetr << ": build kd-tree for tetrode " << tetr << ", " << n_pf_built_ << " / " << tetr_info_->tetrodes_number() << " finished... ";
				kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], buffer->feature_space_dims_[tetr]);
				buffer->log_string_stream_ << "done\nt " << tetr << ": cache " << NN_K << " nearest neighbours for each spike in tetrode " << tetr << " (in a separate thread)...\n";

				fitting_jobs_running_[tetr] = true;
				fitting_jobs_[tetr] = new std::thread(&KDClusteringProcessor::build_lax_and_tree_separate, this, tetr);
				buffer->Log();
			}
		}
	}

	// need both speed and PCs
	unsigned int limit = LOAD ? std::max<int>(buffer->spike_buf_pos_unproc_ - 1, 0): MIN(buffer->spike_buf_pos_speed_, (unsigned int)std::max<int>(buffer->spike_buf_pos_unproc_ - 1, 0));
	while(spike_buf_pos_clust_ < limit){
		Spike *spike = buffer->spike_buffer_[spike_buf_pos_clust_];
		const unsigned int tetr = tetr_info_->Translate(buffer->tetr_info_, (unsigned int)spike->tetrode_);
		const unsigned int nfeat = buffer->feature_space_dims_[tetr];

		if (tetr == TetrodesInfo::INVALID_TETRODE){
			spike_buf_pos_clust_ ++;
			continue;
		}

		// wait until place fields are stabilized
		if (spike->pkg_id_ < SAMPLING_DELAY && !LOAD){
			spike_buf_pos_clust_ ++;
			continue;
		}

		// DEBUG
		if (!delay_reached_reported && !LOAD){
			delay_reached_reported = true;
			Log("Sampling delay over : ", SAMPLING_DELAY);
		}

		// wait for enough spikes to estimate the firing rate; beware of the rewind after estimating the FRs
		if (spike->pkg_id_ < FR_ESTIMATE_DELAY && !LOAD && (tetrode_sampling_rates_.size() == 0)){
			spike_buf_pos_clust_ ++;
			continue;
		}

		// estimate firing rates => spike sampling rates if model has not been loaded
		if (tetrode_sampling_rates_.size() == 0 && !LOAD){
			std::stringstream ss;
			ss << "FR estimate delay over (" << FR_ESTIMATE_DELAY << "). Estimate firing rates => sampling rates and start spike collection";
			Log(ss.str());

			// estimate firing rates
			std::vector<unsigned int> spike_numbers_;
			spike_numbers_.resize(tetr_info_->tetrodes_number());
			// TODO which pointer ?
			for (unsigned int i=0; i < buffer->spike_buf_pos_speed_; ++i){
				Spike *spike = buffer->spike_buffer_[i];
				if (spike == NULL || spike->discarded_ || spike->speed < SPEED_THOLD){
					continue;
				}

				// TODO convert tetrode number
				spike_numbers_[spike->tetrode_] ++;
			}

			for (size_t t=0; t < tetr_info_->tetrodes_number(); ++t){
					double firing_rate = spike_numbers_[t] * buffer->SAMPLING_RATE / double(FR_ESTIMATE_DELAY);
					std::stringstream ss;
					ss << "Estimated firing rate for tetrode #" << t << ": " << firing_rate << " spk / sec\n";
					// TODO configrableize expected session duration
					unsigned int est_sec_left = (buffer->input_duration_ - SAMPLING_DELAY) / buffer->SAMPLING_RATE;
					ss << "Estimated remaining data duration: " << est_sec_left / 60 << " min, " << est_sec_left % 60 << " sec\n";
					tetrode_sampling_rates_.push_back(std::max<int>(0, (int)round(est_sec_left * firing_rate / MIN_SPIKES) - 1));
					ss << "\t sampling rate (with speed thold) set to: " << tetrode_sampling_rates_[t] << "\n";
					Log(ss.str());
			}

			// REWIND TO THE FIRST SPIKE AFTER SAMPLING DELAY
			Log("Rewind the pointer to the last spike after the sampling delay");
			Spike *spike = buffer->spike_buffer_[spike_buf_pos_clust_];
			while ((spike == NULL || spike->pkg_id_ > SAMPLING_DELAY) && (spike_buf_pos_clust_ > 0)){
				spike_buf_pos_clust_ --;
				spike = buffer->spike_buffer_[spike_buf_pos_clust_];
			}
		}

		if (spike->speed < SPEED_THOLD || spike->discarded_){
			spike_buf_pos_clust_ ++;
			continue;
		}

		if (!pf_built_[tetr]){
			// to start prediction only after all PFs are available
			last_pred_pkg_id_ = spike->pkg_id_;

			if ((total_spikes_[tetr] >= MIN_SPIKES && RUN_KDE_ON_MIN_COLLECTED) || buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER){
				// build the kd-tree and call kNN for all points, cache indices (unsigned short ??) in the array of pointers to spikes

				// start clustering if it is not running yet, otherwise - ignore
				if (!fitting_jobs_running_[tetr]){
//					build_lax_and_tree_separate(tetr);
					// SHOULD BE DONE IN THE SAME PROCESS, AS KD Search is not parallel
					// dump required data and start process (due to non-thread-safety of ANN)
					// build tree and dump it along with points
					buffer->log_string_stream_ << "t " << tetr << ": build kd-tree for tetrode " << tetr << ", " << n_pf_built_ << " / " << tetr_info_->tetrodes_number() << " finished... ";
					kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], nfeat);
					buffer->log_string_stream_ << "done\nt " << tetr << ": cache " << NN_K << " nearest neighbours for each spike in tetrode " << tetr << " (in a separate thread)...\n";
					buffer->Log();

					fitting_jobs_running_[tetr] = true;
					fitting_jobs_[tetr] = new std::thread(&KDClusteringProcessor::build_lax_and_tree_separate, this, tetr);
				}
			}
			else{

				if (use_intervals_){
					// if after current interval -> advance interval pointer
					if (current_interval_ < interval_starts_.size() && spike->pkg_id_ > interval_ends_[current_interval_]) {
						Log("Advance to next interval with spike at ", spike->pkg_id_);
						current_interval_ ++;
					}

					// out of intervals
					if (current_interval_ >= interval_starts_.size()) {
						spike_buf_pos_clust_ ++;
						continue;
					}

					if (spike->pkg_id_ < interval_starts_[current_interval_]) {
						spike_buf_pos_clust_ ++;
						continue;
					}
				}

				// sample every SAMLING_RATE spikes for KDE estimation
				if (missed_spikes_[tetr] < tetrode_sampling_rates_[tetr]){
					missed_spikes_[tetr] ++;
					spike_buf_pos_clust_ ++;
					continue;
				}
				missed_spikes_[tetr] = 0;

				obs_spikes_[tetr].push_back(spike);

				// copy features and coords to ann_points_int and obs_mats
				for(unsigned int fet=0; fet < nfeat; ++fet){
					ann_points_[tetr][total_spikes_[tetr]][fet] = spike->pc[fet];

					// save integer with increased precision for integer KDE operations
					ann_points_int_[tetr][total_spikes_[tetr]][fet] = (int)round(spike->pc[fet] * MULT_INT);

					// set from the obs_mats after computing the coordinates normalizing factor
//					spike_coords_int_[tetr](total_spikes_[tetr], 0) = (int)round(spike->x * MULT_INT);
//					spike_coords_int_[tetr](total_spikes_[tetr], 1) = (int)round(spike->y * MULT_INT);

					// tmp: for stats
					obs_mats_[tetr](total_spikes_[tetr], fet) = spike->pc[fet];
				}

				if (spike->x > 0){
					obs_mats_[tetr](total_spikes_[tetr], nfeat) = spike->x;
					obs_mats_[tetr](total_spikes_[tetr], nfeat + 1) = spike->y;
				}
				else{
					obs_mats_[tetr](total_spikes_[tetr], nfeat) = 1023;
					obs_mats_[tetr](total_spikes_[tetr], nfeat + 1) = 1023;
				}

				total_spikes_[tetr] ++;
			}

			spike_buf_pos_clust_ ++;
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
			if (n_pf_built_ < tetr_info_->tetrodes_number()){
				spike_buf_pos_clust_ ++;
				continue;
			}

			// prediction only after reaching delay (place field stability, cross-validation etc.)
			if (buffer->spike_buffer_[spike_buf_pos_clust_]->pkg_id_ < PREDICTION_DELAY){
				spike_buf_pos_clust_ ++;
				continue;
			} else if (!prediction_delay_reached_reported){
				buffer->log_string_stream_ << "Prediction delay over (" << PREDICTION_DELAY << ").\n";
				buffer->Log();
				prediction_delay_reached_reported = true;

				last_pred_pkg_id_ = PREDICTION_DELAY;
				buffer->last_preidction_window_ends_[processor_number_] = PREDICTION_DELAY;
			}

			// also rewind SWRs
			// TODO configurableize
			while (swr_pointer_ < buffer->swrs_.size() && buffer->swrs_[swr_pointer_][2] < PREDICTION_DELAY){
				swr_pointer_ ++;
			}

			// check if NEW swr was detected and has to switch to the SWR regime
			if (SWR_SWITCH){
				if (!swr_regime_ && swr_pointer_ < buffer->swrs_.size() && buffer->swrs_[swr_pointer_][0] != last_processed_swr_start_){
					//DEBUG
					buffer->log_string_stream_ << "Switch to SWR prediction regime due to SWR detected at " << buffer->swrs_[swr_pointer_][0] << ", SWR length = "
							<< (buffer->swrs_[swr_pointer_][2] - buffer->swrs_[swr_pointer_][0]) * 1000 / buffer->SAMPLING_RATE << " ms\n";
					buffer->Log();

					swr_regime_ = true;

					PRED_WIN = SWR_PRED_WIN > 0 ? SWR_PRED_WIN : (buffer->swrs_[swr_pointer_][2] - buffer->swrs_[swr_pointer_][0]);

					last_pred_pkg_id_ = buffer->swrs_[swr_pointer_][0];
					last_processed_swr_start_ = buffer->swrs_[swr_pointer_][0];
					last_window_n_spikes_ = 0;

					// the following is required due to different possible order of SWR detection / spike processing

					// if spikes have been processed before SWR detection - rewind until the first spike in the SW
					while(spike_buf_pos_clust_ > 0 && spike !=nullptr && spike->pkg_id_ > last_pred_pkg_id_){
						spike_buf_pos_clust_ --;
						spike = buffer->spike_buffer_[spike_buf_pos_clust_];
					}

					// !!! if spikes have not been processed yet - rewind until the first spieks in the SWR
					while(spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_ && spike != nullptr && spike->pkg_id_ < last_pred_pkg_id_){
						spike_buf_pos_clust_ ++;
						spike = buffer->spike_buffer_[spike_buf_pos_clust_];
					}

					// reset HMM
					reset_hmm();
				}

				// end SWR regime if the SWR is over
				if (swr_regime_ && last_pred_pkg_id_ > buffer->swrs_[swr_pointer_][2]){
					// DEBUG
					buffer->Log("Switch to theta prediction regime due to end of SWR at ",  (int)buffer->swrs_[swr_pointer_][2]);
					swr_regime_ = false;
					PRED_WIN = THETA_PRED_WIN;

					// reset HMM
					reset_hmm();
					swr_pointer_ ++;
					swr_counter_ ++;
					swr_win_counter_ = 0;
				}
			}

			ANNpoint pnt = annAllocPt(nfeat);
			double dist;
			int closest_ind;
			// at this points all tetrodes have pfs !
			// TODO don't need speed (for prediction), can take more spikes

			while(spike->pkg_id_ < last_pred_pkg_id_ + PRED_WIN && spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_ - 1){
				const unsigned int stetr = tetr_info_->Translate(buffer->tetr_info_, spike->tetrode_);

				if (stetr == TetrodesInfo::INVALID_TETRODE || spike->discarded_){
					spike_buf_pos_clust_++;
					// spike may be without speed (the last one) - but it's not crucial
					spike = buffer->spike_buffer_[spike_buf_pos_clust_];
					continue;
				}

				// in the SWR regime - skip until the first spike in the SWR
				if (swr_regime_ && spike->pkg_id_ < last_processed_swr_start_){
					spike_buf_pos_clust_++;
					spike = buffer->spike_buffer_[spike_buf_pos_clust_];
					continue;
				}

				tetr_spiked_[stetr] = true;

				for(unsigned int fet=0; fet < nfeat; ++fet){
					pnt[fet] = spike->pc[fet];
				}

				last_window_n_spikes_ ++;

				// PROFILE
//				time_t kds = clock();
				// 5 us for eps = 0.1, 20 ms - for eps = 10.0, prediction quality - ???
				// TODO : quantify dependence of prediction quality on the EPS
				kdtrees_[stetr]->annkSearch(pnt, 1, &closest_ind, &dist, NN_EPS);
//				std::cout << "kd time = " << clock() - kds << "\n";

				// add 'place field' of the spike with the closest wave shape
				pos_pred_ += laxs_[stetr][closest_ind];

				spike_buf_pos_clust_++;
				// spike may be without speed (the last one) - but it's not crucial
				spike = buffer->spike_buffer_[spike_buf_pos_clust_];
			}

			// if have to wait until the speed estimate
			if(WAIT_FOR_SPEED_EST && (unsigned int)(last_pred_pkg_id_ / (float)POS_SAMPLING_RATE) >= buffer->pos_buf_pos_speed_est){
				return;
			}

			// if prediction is final and end of window has been reached (last spike is beyond the window)
			// 		or prediction will be finalized in subsequent iterations
			if(spike->pkg_id_ >= last_pred_pkg_id_ + PRED_WIN){

				// DEBUG
				buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Time from after package extraction until arrival in KD at the prediction start\n");

				// edges of the window
				// account for the increase in the firing rate during high synchrony with additional factor
				const double DE_SEC = PRED_WIN / (float)buffer->SAMPLING_RATE * (swr_regime_ ? 5.0 : 1.0);

				for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
					if (tetr_spiked_[t]){
						// TODO: depricated LX_WEIGHT, was introduced only for debugging purposes
						pos_pred_ -= DE_SEC  * lxs_[t];
					}
				}

				// DUMP decoded coordinate
				unsigned int mx = 0,my = 0;
				pos_pred_.max(mx, my);
				unsigned int gtx = buffer->pos_unknown_, gty = buffer->pos_unknown_;
				// TODO extract to get pos
				// !!! WORKAROUND
				SpatialInfo &pose = buffer->positions_buf_[(unsigned int)(last_pred_pkg_id_ / (float)POS_SAMPLING_RATE)];
				if (pose.x_big_LED_ == buffer->pos_unknown_){
					if (pose.x_small_LED_ != buffer->pos_unknown_){
						gtx = pose.x_small_LED_;
						gty = pose.y_small_LED_;
					}
				} else if (pose.x_small_LED_ == buffer->pos_unknown_){
					if (pose.x_big_LED_ != buffer->pos_unknown_){
						gtx = pose.x_big_LED_;
						gty = pose.y_big_LED_;
					}
				}
				else {
					gtx = pose.x_pos();
					gty = pose.y_pos();
				}

				// DEBUG
				if (last_pred_pkg_id_ > DUMP_DELAY && !dump_delay_reach_reported_){
					dump_delay_reach_reported_= true;
					Log("Dump delay reached: ", (int)DUMP_DELAY);
				}
				if (last_pred_pkg_id_ > DUMP_END && !dump_end_reach_reported_){
					dump_end_reach_reported_= true;
					Log("Dump end reached: ", (int)DUMP_END);
				}

				if (last_pred_pkg_id_ > DUMP_DELAY && last_pred_pkg_id_ < DUMP_END && pose.speed_ >= DUMP_SPEED_THOLD){
					dec_bayesian_ << BIN_SIZE * (mx + 0.5) << " " << BIN_SIZE * (my + 0.5) << " " << gtx << " " << gty << "\n";
					dec_bayesian_.flush();
				}

				// DEBUG
//				std::cout << "Searching at " << (unsigned int)(last_pred_pkg_id_ / (float)POS_SAMPLING_RATE) << " while speed_est at " << buffer->pos_buf_pos_speed_est << ", speed = " << pose[5] << "\n";

				//DEBUG - slow down to see SWR prediction
				if (swr_regime_){
					buffer->log_string_stream_ << swr_win_counter_ << "-th window, prediction within SWR..., proc# = " << processor_number_ << "# of spikes = " << last_window_n_spikes_ << "\n";
					buffer->Log();

					if (buffer->last_pkg_id > SWR_SLOWDOWN_DELAY){
						usleep(1000 * SWR_SLOWDOWN_DURATION);
					}

					window_spike_counts_ << last_window_n_spikes_ << "\n";
					window_spike_counts_.flush();
				}

				// DEBUG
				// std::cout << "Number of spikes in the prediction window: " << last_window_n_spikes_ << " (proc# " << processor_number_ << ")\n";
				last_window_n_spikes_ = 0;

				last_pred_probs_ = pos_pred_;
				// to avoid OOR
				double minpred = arma::max(arma::max(last_pred_probs_));
				last_pred_probs_ -= minpred;

				if (swr_regime_){
					// DEBUG save prediction
					pos_pred_.save("../out/jc118_1003/" + std::string("swr_") + Utils::Converter::int2str(swr_counter_) + "_" + Utils::Converter::int2str(swr_win_counter_) + "_" + Utils::Converter::int2str(processor_number_) + ".mat", arma::raw_ascii);
					swr_win_counter_ ++;
				}
				else{
					//pos_pred_.save("../out/jc84_1910/" + std::string("learn_") + Utils::Converter::int2str(swr_win_counter_) + ".mat", arma::raw_ascii);
					//swr_win_counter_ ++;
				}

				// THE POINT AT WHICH THE PREDICTION IS READY
				// DEBUG
				buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Time from after package extraction until prediction for the window ready\n");

//				double minval = arma::min(arma::min(pos_pred_));
//				pos_pred_ = pos_pred_ - minval;
				pos_pred_ = arma::exp(pos_pred_ / 50);

				// updated in HMM
				buffer->last_predictions_[processor_number_] = pos_pred_.t();
				buffer->last_preidction_window_ends_[processor_number_] = last_pred_pkg_id_ + PRED_WIN;

				// DEBUG
//				if (!(last_pred_pkg_id_ % 200)){
//					pos_pred_.save(BASE_PATH + "tmp_pred_" + Utils::Converter::int2str(last_pred_pkg_id_) + ".mat", arma::raw_ascii);
//					std::cout << "save prediction...\n";
//				}

				last_pred_pkg_id_ += PRED_WIN;

				// re-init prediction variables
				tetr_spiked_ = std::vector<bool>(tetr_info_->tetrodes_number(), false);

				if (USE_HMM)
					update_hmm_prediction();

				// DEBUG
				npred ++;
				if (!(npred % 2000)){
					buffer->log_string_stream_ << "Bayesian prediction window bias control: " << npred << " / " << (int)round((last_pred_pkg_id_ - PREDICTION_DELAY) / (float)PRED_WIN) << "\n";
					buffer->Log();
				}

				// TODO: extract
				pos_pred_ = USE_PRIOR ? (tetr_info_->tetrodes_number() * pix_log_) : arma::mat(NBINSX, NBINSY, arma::fill::zeros);

				// return to display prediction etc...
				//		(don't need more spikes at this stage)
				return;
			}
			// marginal rate function at each tetrode
		}
	}

	// WORKAROUND
	if (processor_number_ == 0){
		buffer->spike_buf_pos_clust_ = spike_buf_pos_clust_;
	}
}

void KDClusteringProcessor::build_lax_and_tree_separate(const unsigned int tetr) {
	std::ofstream kdstream(BASE_PATH + Utils::Converter::int2str(tetr) + ".kdtree");
	kdtrees_[tetr]->Dump(ANNtrue, kdstream);
	kdstream.close();

	// dump obs_mat
	obs_mats_[tetr].resize(total_spikes_[tetr], obs_mats_[tetr].n_cols);
	obs_mats_[tetr].save(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_obs.mat");

	// free resources
	obs_mats_[tetr].clear();
	delete kdtrees_[tetr];

	// create pos_buf and dump (first count points),
	// TODO cut matrix later
	unsigned int npoints = 0;
	// TODO sparse sampling
	arma::Mat<float> pos_buf(2, buffer->pos_buf_pos_);
	unsigned int pos_interval = 0;
	int nskip = 0;
	for (unsigned int n = 0; n < buffer->pos_buf_pos_; ++n) {
		if (buffer->positions_buf_[n].speed_ < SPEED_THOLD){
			nskip ++;
		}

		// if pos is unknown or speed is below the threshold - ignore
		if ( buffer->positions_buf_[n].x_small_LED_ == 1023 || buffer->positions_buf_[n].x_big_LED_ == 1023 || buffer->positions_buf_[n].x_big_LED_ < 0 || buffer->positions_buf_[n].x_small_LED_ < 0 || buffer->positions_buf_[n].speed_ < SPEED_THOLD){
			continue;
		}

		// skip if out of the intervals
		if (use_intervals_){
			// TODO account for buffer rewinds
			unsigned int pos_time = POS_SAMPLING_RATE * n;
			while (pos_interval < interval_starts_.size() && pos_time > interval_ends_[pos_interval]){
				pos_interval ++;
			}

			if (pos_interval >= interval_starts_.size()){
				continue;
			}

			if (pos_time < interval_starts_[pos_interval]){
				continue;
			}
		}

		pos_buf(0, npoints) = buffer->positions_buf_[n].x_pos();
		pos_buf(1, npoints) = buffer->positions_buf_[n].y_pos();
		npoints++;
	}
	Log("Skipped due to speed: ", nskip);
	pos_buf = pos_buf.cols(0, npoints - 1);
	pos_buf.save(BASE_PATH  + "tmp_" + Utils::NUMBERS[tetr] + "_pos_buf.mat");

	unsigned int last_pkg_id = buffer->last_pkg_id;
	// if using intervals, provide sum of interval lengthes until last_pkg_id
	if (use_intervals_){
		last_pkg_id = 0;
		for (unsigned int i = 0; i < current_interval_; ++i) {
			last_pkg_id += interval_ends_[i] - interval_starts_[i];
		}
		if (current_interval_ < interval_starts_.size()) // && buffer->last_pkg_id < interval_ends_[current_interval_])
			last_pkg_id += buffer->last_pkg_id - interval_starts_[current_interval_];

		Log("Due to interval usage, calling KDE with ", (int)last_pkg_id);
		Log("	while the real last_pkg_id is ", (int)buffer->last_pkg_id);
	}

	/// buuild commandline to start kde_estimator
	std::ostringstream  os;
	const unsigned int nfeat = buffer->feature_space_dims_[tetr];
	os << kde_path_ << " " << tetr << " " << nfeat << " " << NN_K << " " << NN_K_COORDS << " " << nfeat << " " <<
			MULT_INT  << " " << NBINSX << " " << NBINSY << " " << total_spikes_[tetr] << " " <<
			tetrode_sampling_rates_[tetr] + 1 << " " << buffer->SAMPLING_RATE << " " << last_pkg_id << " " << SAMPLING_DELAY << " " << BIN_SIZE << " " << NN_EPS
			<< " " << SIGMA_X << " " << SIGMA_A << " " << SIGMA_XX << " " << SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD << " " << SPIKE_GRAPH_COVER_NNEIGHB << " " << BASE_PATH;
	buffer->log_string_stream_ << "t " << tetr << ": Start external kde_estimator with command (tetrode, dim, nn_k, nn_k_coords, n_feat, mult_int,  bin_size, n_bins. min_spikes, sampling_rate, buffer_sampling_rate, last_pkg_id, sampling_delay, nn_eps, sigma_x, sigma_a, sigma_xx, vc_dist_thold, vc_nneighb)\n\t" << os.str() << "\n";
	buffer->Log();

//	if (tetr == 13)
	int retval = system(os.str().c_str());
	if (retval < 0){
		buffer->Log("ERROR: impossible to start kde_estimator!");
	}
	else{
		buffer->log_string_stream_ << "t " << tetr << ": kde estimator exited with code " << retval << "\n";
		buffer->Log();
	}

	pf_built_[tetr] = true;

	// TODO competitive
	n_pf_built_ ++;

	if (n_pf_built_ == tetr_info_->tetrodes_number()){
		Log("KDE at all tetrodes done, exiting...\n");
		exit(0);
	}
}

void KDClusteringProcessor::JoinKDETasks(){
    for(size_t t=0; t < tetr_info_->tetrodes_number(); ++t){
        if(fitting_jobs_running_[t])
            fitting_jobs_[t]->join();
    }
    buffer->Log("All KDE jobs joined...");
}

std::string KDClusteringProcessor::name(){
	return "KD Clustering";
}
