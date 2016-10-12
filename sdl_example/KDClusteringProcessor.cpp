/*
 * KDClusteringProcessor.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: Igor Gridchyn
 */

#include "KDClusteringProcessor.h"
#include "Utils.h"

#include <fstream>

void KDClusteringProcessor::load_laxs_tetrode(unsigned int t){
	buffer->Log("Load probability estimations for tetrode ", t);

	// load l(a,x) for all spikes of the tetrode
	//			for (int i = 0; i < MIN_SPIKES; ++i) {
	//				laxs_[t][i].load(BASE_PATH + Utils::NUMBERS[t] + "_" + Utils::Converter::int2str(i) + ".mat");
	//			}

	// 	beware of inexation changes
	std::string rtreepath = BASE_PATH + Utils::NUMBERS[t] + ".kdtree.reduced";
	Utils::FS::CheckFileExistsWithError(rtreepath, (Utils::Logger*) this);
	std::ifstream kdtree_stream(rtreepath);
	kdtrees_[t] = new ANNkd_tree(kdtree_stream);
	kdtree_stream.close();

	int NUSED = kdtrees_[t]->nPoints();
	laxs_[t].reserve(NUSED);

	// load binary combined matrix and extract individual l(a,x)
	arma::fmat laxs_tetr_(NBINSX, NBINSY * NUSED);
	std::string laxs_path = BASE_PATH + Utils::NUMBERS[t] + "_tetr.mat";
	Utils::FS::CheckFileExistsWithError(laxs_path, (Utils::Logger*) this);
	laxs_tetr_.load(laxs_path);
	for (int s = 0; s < NUSED; ++s) {
		laxs_[t].push_back(laxs_tetr_.cols(s * NBINSY, (s + 1) * NBINSY - 1));

		if (!(s % 50))
			laxs_[t][laxs_[t].size() - 1].save(
					BASE_PATH + Utils::NUMBERS[t] + "_"
							+ Utils::Converter::int2str(s) + ".tmp",
					arma::raw_ascii);

		// CHECK FOR NANS
		arma::fmat & smat = laxs_[t][laxs_[t].size() - 1];
		float smatmin = std::numeric_limits<float>::min();
		for (unsigned int bx = 0; bx < NBINSX; ++bx){
			for (unsigned int by = 0; by < NBINSY; ++by){
				if (!std::isinf(smat(bx,by)) && smat(bx, by) > smatmin){
					smatmin = smat(bx, by);
				}
			}
		}
		for (unsigned int bx = 0; bx < NBINSX; ++bx){
			for (unsigned int by = 0; by < NBINSY; ++by){
				if (std::isinf(smat(bx, by))){
					smat(bx,by) = smatmin;
				}
			}
		}
	}
	//laxs_tetr_.save(BASE_PATH + Utils::NUMBERS[t] + "_tetr.mat");

	// load marginal rate function
	std::string lxs_path = BASE_PATH + Utils::NUMBERS[t] + "_lx.mat";
	Utils::FS::CheckFileExistsWithError(lxs_path, (Utils::Logger*) this);
	lxs_[t].load(lxs_path);
	float maxval = arma::max(arma::max(lxs_[t]));
	for (unsigned int xb = 0; xb < NBINSX; ++xb) {
		for (unsigned int yb = 0; yb < NBINSY; ++yb) {
			if (lxs_[t](xb, yb) == 0) {
				lxs_[t](xb, yb) = maxval;
			}
		}
	}

	std::string pxs_path = BASE_PATH + Utils::NUMBERS[t] + "_lx.mat";
	Utils::FS::CheckFileExistsWithError(pxs_path, (Utils::Logger*) this);
	pxs_[t].load(pxs_path);

	pf_built_[t] = true;
}

KDClusteringProcessor::KDClusteringProcessor(LFPBuffer* buf,
		const unsigned int& processor_number) :
		LFPProcessor(buf, processor_number), MIN_SPIKES(
				getInt("kd.min.spikes")), BASE_PATH(getOutPath("kd.path.base")), SAMPLING_DELAY(
				getInt("kd.sampling.delay")), SAMPLING_END(
				buf->config_->getInt("kd.sampling.end",
						std::numeric_limits<int>::max())), SAVE(
				getBool("kd.save")), LOAD(!getBool("kd.save")), USE_PRIOR(
				getBool("kd.use.prior")), SAMPLING_RATE(
				getInt("kd.sampling.rate")), SPEED_THOLD(
				getFloat("kd.speed.thold")), NN_EPS(getFloat("kd.nn.eps")), USE_HMM(
				getBool("kd.use.hmm")), NBINSX(getInt("nbinsx")), NBINSY(
				getInt("nbinsy")), BIN_SIZE(getFloat("bin.size")), HMM_NEIGHB_RAD(
				getInt("kd.hmm.neighb.rad")), PREDICTION_DELAY(
				getInt("kd.prediction.delay")), NN_K(getInt("kd.nn.k")), NN_K_COORDS(
				getInt("kd.nn.k.space")), MULT_INT(getInt("kd.mult.int")), SIGMA_X(
				getFloat("kd.sigma.x")), SIGMA_A(getFloat("kd.sigma.a")), SIGMA_XX(
				getFloat("kd.sigma.xx")), SWR_SWITCH(
				buf->config_->getBool("kd.swr.switch")), SWR_SLOWDOWN_DELAY(
				buf->config_->getInt("kd.swr.slowdown.delay", 0)), SWR_SLOWDOWN_DURATION(
				buf->config_->getInt("kd.swr.slowdown.duration", 1500)), SWR_PRED_WIN(
				buf->config_->getInt("kd.swr.pred.win", 400)), DUMP_DELAY(
				buf->config_->getInt("kd.dump.delay", 46000000)), DUMP_END(
				buf->config_->getInt("kd.dump.end", 1000000000)), DUMP_END_EXIT(
				buf->config_->getBool("kd.dump.end.exit", false)), HMM_RESET_RATE(
				buf->config_->getInt("kd.hmm.reset.rate", 60000000)), use_intervals_(
				buf->config_->getBool("kd.use.intervals", false)), spike_buf_pos_clust_(
				buf->spike_buf_pos_clusts_[processor_number]), THETA_PRED_WIN(
				buf->config_->getInt("kd.pred.win", 2400)), SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD(
				buf->config_->getFloat(
						"kd.spike.graph.cover.distance.threshold", 0)), SPIKE_GRAPH_COVER_NNEIGHB(
				buf->config_->getInt("kd.spike.graph.cover.nneighb", 1)), FR_ESTIMATE_DELAY(
				buf->config_->getInt("kd.frest.delay", 1000000)), DUMP_SPEED_THOLD(
				buf->config_->getFloat("kd.dump.speed.thold", .0)), WAIT_FOR_SPEED_EST(
				getBool("kd.wait.speed")), RUN_KDE_ON_MIN_COLLECTED(
				getBool("kd.run.on.min")), kde_path_(
				buf->config_->getString("kd.path", "./kde_estimator")), swr_dec_dump_path_(
				buf->config_->getOutPath("kd.swrdec.path", std::string("swr_dec_dump") + buf->config_->timestamp_ + ".txt")), continuous_prediction_(
				buf->config_->getBool("kd.pred.continuous", false)), neighb_num_(
				buf->config_->getInt("kd.neighb.num", 1)), display_scale_(
				buf->config_->getInt("kd.display.scale", 50)), SWR_COMPRESSION_FACTOR(
				buf->config_->getFloat("kd.swr.compression.factor", 5.0)), pred_dump_(
				buf->config_->getBool("kd.pred.dump", false)), pred_dump_pref_(
				buf->config_->getOutPath("kd.pred.dump.pref", "pred_")), spike_buf_pos_pred_start_(
				buf->spike_buf_pos_pred_start_), prediction_window_spike_number_(
				buf->config_->getInt("kd.fixed.spike.number")), prediction_windows_overlap_(
				buf->config_->getInt(
						"kd.prediction.windows.overlap.percentage")
						* prediction_window_spike_number_ / 100), BINARY_CLASSIFIER(
				buf->config_->getBool("kd.binary", false)), MAX_KDE_JOBS(
				buf->config_->getInt("kd.max.jobs", 5)), SINGLE_PRED_PER_SWR(
				buf->config_->getBool("kd.single.pred.per.swr", false)), IGNORE_LX(
				buf->config_->getBool("kd.ignore.lx", false)), INTERLEAVING_WINDOWS(
				buf->config_->getBool("kd.interleaving.windows", false)), MIN_POS_SAMPLES(
				buf->config_->getInt("kd.min.pos,samples", 1000)), KD_MIN_OCC(
				buf->config_->getFloat("kd.min.occ", .001))
	{

	Log("Construction started");

	Log("Prediction windows overlap = ", prediction_windows_overlap_);

	pf_dumped_.resize(100000, false);

	PRED_WIN = THETA_PRED_WIN;

	// load proper tetrode info
	if (buf->alt_tetr_infos_.size() < processor_number_ + 1) {
		tetr_info_ = buf->alt_tetr_infos_[0];
		Log("WARNING: using default tetrode info for processor number",
				(int) processor_number_);
	} else {
		tetr_info_ = buf->alt_tetr_infos_[processor_number_];
	}

	// which tetrodes out of the tetrode config are used to build the model / decode
	use_tetrode_.resize(buffer->tetr_info_->tetrodes_number(), false);
	for (unsigned int i = 0; i < buffer->config_->kd_tetrodes_.size(); ++i){
		use_tetrode_[buffer->config_->kd_tetrodes_[i]] = true;
	}
	if (buffer->config_->kd_tetrodes_.size() == 0){
		for (unsigned int i = 0; i < buffer->tetr_info_->tetrodes_number(); ++i){
			use_tetrode_[i] = true;
			buffer->config_->kd_tetrodes_.push_back(i);
		}
	}
	n_pf_built_ =  buffer->tetr_info_->tetrodes_number() - buffer->config_->kd_tetrodes_.size();
	Log("Number of tetrodes to be used in the KD processor: ", (int)buffer->config_->kd_tetrodes_.size());


	const unsigned int tetrn = tetr_info_->tetrodes_number();
	kdtrees_.resize(tetrn);
	ann_points_.resize(tetrn);
	total_spikes_.resize(tetrn);
	knn_cache_.resize(tetrn);
	spike_place_fields_.resize(tetrn);
	obs_mats_.resize(tetrn);
	missed_spikes_.resize(tetrn);

	kdtrees_coords_.resize(tetrn);

	fitting_jobs_.resize(tetrn, nullptr);
	fitting_jobs_running_.resize(tetrn, false);

	Log("Allocate distribution matrices");
	laxs_.resize(tetrn);
	pxs_.resize(tetrn, arma::fmat(NBINSX, NBINSY, arma::fill::zeros));
	lxs_.resize(tetrn, arma::fmat(NBINSX, NBINSY, arma::fill::zeros));

	unsigned int max_dim = 0;

	Log("Allocate observation arrays");
	for (unsigned int t = 0; t < tetrn; ++t) {
		const unsigned int dim = buf->feature_space_dims_[t];

		// TODO !!! allow to be extended
		unsigned int maxPoints = MIN_SPIKES * 3;

		if (use_tetrode_[t]){
			ann_points_[t] = annAllocPts(maxPoints, dim);
			spike_place_fields_[t].reserve(maxPoints);

			// tmp
			obs_mats_[t] = arma::fmat(maxPoints,
					buffer->feature_space_dims_[t] + 2);

			if (buffer->feature_space_dims_[t] > max_dim)
				max_dim = buffer->feature_space_dims_[t];
		}
	}

	pf_built_.resize(tetrn);

	pix_log_ = arma::fmat(NBINSX, NBINSY, arma::fill::zeros);

	if (LOAD) {
		// load occupancy
		Utils::FS::CheckFileExistsWithError(BASE_PATH + "pix_log.mat", (Utils::Logger*) this);
		pix_log_.load(BASE_PATH + "pix_log.mat");

		if (pix_log_.n_rows != NBINSX || pix_log_.n_cols!= NBINSY){
			buffer->processing_over_ = true;
			Log("ERROR: NBINSX / NBINSY mismatch");
			return;
		}

		for (unsigned int t = 0; t < tetrn; ++t) {
			if (use_tetrode_[t])
				load_laxs_tetrode(t);
		}

		n_pf_built_ = tetrn;
	}

	if (USE_PRIOR) {
		// will be 0's if not loaded
		pos_pred_ = pix_log_;
	} else {
		pos_pred_ = arma::fmat(NBINSX, NBINSY, arma::fill::zeros);
	}

	tetr_spiked_ = std::vector<bool>(tetr_info_->tetrodes_number(), false);
	// posterior position probabilities map
	// initialize with log of prior = pi(x)

	reset_hmm();

	hmm_traj_.resize(NBINSX * NBINSY);

	// save params only for model building scenario
	if (SAVE) {
		std::string parpath = BASE_PATH + "params.txt";
		std::ofstream fparams(parpath);
		fparams
				<< "SIGMA_X, SIGMA_A, SIGMA_XX, MULT_INT, SAMPLING_RATE, NN_K, NN_K_SPACE(obsolete), MIN_SPIKES, SAMPLING_RATE, SAMPLING_DELAY, NBINSX, NBINSY, BIN_SIZE, SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD, SPIKE_GRAPH_COVER_NNEIGHB, NN_EPS, MIN_OCC\n"
				<< SIGMA_X << " " << SIGMA_A << " " << SIGMA_XX << " "
				<< MULT_INT << " " << SAMPLING_RATE << " " << NN_K << " "
				<< NN_K_COORDS << " " << MIN_SPIKES << " " << SAMPLING_RATE
				<< " " << SAMPLING_DELAY << " " << NBINSX << " " << NBINSY
				<< " " << BIN_SIZE << " "
				<< SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD << " "
				<< SPIKE_GRAPH_COVER_NNEIGHB << " " << NN_EPS << " " << KD_MIN_OCC << "\n";
		fparams.close();
		Log(std::string("Running params written to") + parpath);

		// save all params together with the model directory
		std::string all_params = buffer->config_->getAllParamsText();
		std::ofstream model_config(BASE_PATH + "params.conf");
		model_config << all_params;
		model_config.close();
	}

	if (use_intervals_) {
		std::string intpath = buf->config_->getString("kd.intervals.path");
		std::ifstream intfile(intpath);
		while (!intfile.eof()) {
			int ints = 0, inte = 0;
			intfile >> ints >> inte;
			interval_starts_.push_back(ints);
			interval_ends_.push_back(inte);
		}
		intfile.close();
	}

	buffer->last_predictions_.resize(processor_number_ + 1);
	// initialize prediction in the buffer
	buffer->last_predictions_[processor_number] = pos_pred_;

	dec_bayesian_.open("dec_bay.txt");
	dec_bayesian_ << buf->config_->getString("model.id") << "\n";
	//window_spike_counts_.open("../out/window_spike_counts.txt");

	pnt_ = annAllocPt(max_dim);

	Log("Construction ready");

	// DEBUG
//	skipped_spikes_.resize(tetr_info_->tetrodes_number(), 0);
//	debug_.open("debug.txt");

	neighbour_dists_.resize(neighb_num_);
	neighbour_inds_.resize(neighb_num_);

	if (continuous_prediction_ && !IGNORE_LX) {
		// from last spike at the current tetrode
		const float DE_SEC = float((prediction_window_spike_number_ > 0) ? (THETA_PRED_WIN / (float) buffer->SAMPLING_RATE ) :( PRED_WIN / (float) buffer->SAMPLING_RATE * (swr_regime_ ? SWR_COMPRESSION_FACTOR : 1.0)));

		for (unsigned int stetr = 0; stetr < tetr_info_->tetrodes_number(); stetr++)
			if (use_tetrode_[stetr])
				pos_pred_ -= DE_SEC * lxs_[stetr];
	}

	if (pred_dump_){
		if (Utils::FS::FileExists(swr_dec_dump_path_)){
			Log("ERROR: SWR DUMP FILE EXISTS, EXITING!");
			Log(swr_dec_dump_path_);
			exit(235453);
		}

		swr_dec_dump_.open(swr_dec_dump_path_);
	}

	prediction_skipped_spikes_.resize(tetr_info_->tetrodes_number(), 0);
	window_spikes_.resize(tetr_info_->tetrodes_number(), 0);
}

KDClusteringProcessor::~KDClusteringProcessor() {
}

const arma::fmat& KDClusteringProcessor::GetPrediction() {
	return last_pred_probs_;
}

void KDClusteringProcessor::update_hmm_prediction() {
	// old hmm_prediction + transition (without evidence)
	arma::fmat hmm_upd_(NBINSX, NBINSY, arma::fill::zeros);

	// TODO CONTROLLED reset, PARAMETRIZE
	unsigned int prevmaxx = 0, prevmaxy = 0;

	bool reset = buffer->last_preidction_window_ends_[processor_number_]
			- last_hmm_reset_ > HMM_RESET_RATE;
	if (reset) {
		// DEBUG
		buffer->Log("Reset HMM at ",
				(int) buffer->last_preidction_window_ends_[processor_number_]);

		hmm_prediction_.max(prevmaxx, prevmaxy);

		if (USE_PRIOR) {
			hmm_prediction_ = pix_log_;
		} else {
			hmm_prediction_ = arma::fmat(NBINSX, NBINSY, arma::fill::zeros);
		}

		last_hmm_reset_ =
				buffer->last_preidction_window_ends_[processor_number_];
	}

	for (unsigned int xb = 0; xb < NBINSX; ++xb) {
		for (unsigned int yb = 0; yb < NBINSY; ++yb) {
			// find best (x, y) - with highest probability of (x,y)->(xb,yb)
			// implement hmm{t}(xb,yb) = max_{x,y \in neighb(xb,yb)}(hmm_{t-1}(x,y) * tp(x,y,xb,yb) * prob(xb, yb | a_{1..N}))

			float best_to_xb_yb = -1000.0f * 100000000.0f;
			int bestx = 0, besty = 0;

			for (unsigned int x = (unsigned int)std::max<int>(0, xb - HMM_NEIGHB_RAD); x <= MIN(xb + HMM_NEIGHB_RAD, NBINSX - 1); ++x) {
				for (unsigned int y = (unsigned int)std::max<int>(0, yb - HMM_NEIGHB_RAD); y <= MIN(yb + HMM_NEIGHB_RAD, NBINSY - 1); ++y) {
					// TODO weight
					// split for DEBUG
					float prob_xy = hmm_prediction_(x, y);
					int shx = xb - x + HMM_NEIGHB_RAD;
					int shy = yb - y + HMM_NEIGHB_RAD;

					prob_xy += buffer->tps_[y * NBINSX + x](shx, shy);
					if (prob_xy > best_to_xb_yb) {
						best_to_xb_yb = prob_xy;
						bestx = x;
						besty = y;
					}
				}
			}

			hmm_upd_(xb, yb) = best_to_xb_yb;

			if (reset) {
				// all leading to the best before the reset
				hmm_traj_[yb * NBINSX + xb].push_back(
						prevmaxy * NBINSX + prevmaxx);
			} else {
				hmm_traj_[yb * NBINSX + xb].push_back(besty * NBINSX + bestx);
			}
		}
	}

	// DEBUG - check that no window is skipped and the chain is broken
	if (!(hmm_traj_[0].size() % 2000)) {
		buffer->log_string_stream_ << "hmm bias control: "
				<< hmm_traj_[0].size() << " / "
				<< (int) round(
						(last_pred_pkg_id_ - PREDICTION_DELAY)
								/ (float) PRED_WIN) << "\n";
		buffer->Log();
		// DEBUG
		hmm_prediction_.save(
				BASE_PATH + "hmm_pred_"
						+ Utils::Converter::int2str(hmm_traj_[0].size())
						+ ".mat", arma::raw_ascii);
	}

	// add Bayesian pos likelihood from evidence
	hmm_prediction_ = hmm_upd_ + last_pred_probs_;

	// VISUALIZATION ADJUSTMENT:to avoid overflow
	hmm_prediction_ = hmm_prediction_ - hmm_prediction_.max();

	// DEBUG
//	hmm_prediction_.save(BASE_PATH + "hmm_pred_" + Utils::Converter::int2str(hmm_traj_[0].size()) + ".mat", arma::raw_ascii);

	// STATS - write error of Bayesian and HMM, compare to pos in the middle of the window
	const SpatialInfo& gt_pos = buffer->PositionAt(last_pred_pkg_id_ - PRED_WIN / 2);
	float corrx = gt_pos.x_pos();
	float corry = gt_pos.y_pos();

	// for consistency of comparison
	if (last_pred_pkg_id_ > DUMP_DELAY) {
		// STATS - dump best HMM trajectory by backtracking
		std::ofstream dec_hmm(std::string("dec_hmm_") + Utils::NUMBERS[processor_number_] + ".txt");
		int t = hmm_traj_[0].size() - 1;
		// best last x,y
		unsigned int x, y;
		hmm_prediction_.max(x, y);
		while (t >= 0) {
			dec_hmm << BIN_SIZE * (x + 0.5) << " " << BIN_SIZE * (y + 0.5) << " ";
			const SpatialInfo& gt_pos = buffer->PositionAt(t * PRED_WIN + PREDICTION_DELAY);
			corrx = gt_pos.x_pos();
			corry = gt_pos.y_pos();
			dec_hmm << corrx << " " << corry << "\n";
			unsigned int b = hmm_traj_[y * NBINSX + x][t];

			if (b < NBINSX * NBINSY) {
				y = b / NBINSX;
				x = b % NBINSX;
			} else {
				buffer->log_string_stream_ << "Trajectory is screwed up at " << t << ", keep the previous position\n";
				buffer->Log();
			}

			t--;
		}
		dec_hmm.flush();

		buffer->Log("Exit after dumping the HMM prediction :",
				(int) DUMP_DELAY);
		buffer->processing_over_ = true;
	}

	buffer->last_predictions_[processor_number_] = arma::exp(
			hmm_prediction_ / (float)display_scale_);
}

void KDClusteringProcessor::reset_hmm() {
	hmm_prediction_ = arma::fmat(NBINSX, NBINSY, arma::fill::zeros);
	if (USE_PRIOR) {
		hmm_prediction_ = pix_log_;
	}
}

void KDClusteringProcessor::dump_positoins_if_needed(const unsigned int& mx,
		const unsigned int& my) {
	if (INTERLEAVING_WINDOWS && ((last_pred_pkg_id_ / PRED_WIN) % 2) == 1){
		return;
	}

	if (last_pred_pkg_id_ + PRED_WIN >= DUMP_END && !dump_end_reach_reported_) {
		dump_end_reach_reported_ = true;
		Log("Dump end reached: ", (int) DUMP_END);
		if (DUMP_END_EXIT) {
			Log("EXIT upon dump end reached");
			buffer->processing_over_ = true;
		}
	}

	if (last_pred_pkg_id_ > DUMP_DELAY && last_pred_pkg_id_ < DUMP_END) {
		// DEBUG
		if (!dump_delay_reach_reported_) {
			dump_delay_reach_reported_ = true;
			Log("Dump delay reached: ", (int) DUMP_DELAY);
		}

		float confidence = arma::max(arma::max(pos_pred_.rows(0, NBINSX/2 - 1))) - arma::max(arma::max(pos_pred_.rows(NBINSX/2, NBINSX - 1)));

		float gtx = (float)buffer->pos_unknown_, gty = (float)buffer->pos_unknown_;

		const SpatialInfo &pose = buffer->PositionAt(last_pred_pkg_id_);
		gtx = pose.x_pos();
		gty = pose.y_pos();

		// for output need non-nan value
		if (Utils::Math::Isnan(gtx)) {
			gtx = (float)buffer->pos_unknown_;
			gty = (float)buffer->pos_unknown_;
		}

		if (pose.speed_ >= DUMP_SPEED_THOLD) { // && pose.dirvar_ < 0.5) {
			double mult = BINARY_CLASSIFIER ? 100 : 1.0;
			dec_bayesian_ << BIN_SIZE * (mx + 0.5) * mult << " " << BIN_SIZE * (my + 0.5) * mult << " " << gtx << " " << gty << " " << confidence << "\n";
			dec_bayesian_.flush();
		}
	}
}

void KDClusteringProcessor::dump_swr_window_spike_count() {
	//DEBUG - slow down to see SWR prediction
//				if (swr_regime_){
//					buffer->log_string_stream_ << swr_win_counter_ << "-th window, prediction within SWR..., proc# = " << processor_number_ << "\n";
//					buffer->Log();

	// DEBUG
//					if (buffer->last_pkg_id > SWR_SLOWDOWN_DELAY){
//						usleep(1000 * SWR_SLOWDOWN_DURATION);
//					}

//					window_spike_counts_ << last_window_n_spikes_ << "\n";
//					window_spike_counts_.flush();
//				}
}

void KDClusteringProcessor::dump_prediction_if_needed() {
	if (pred_dump_) {
		if (swr_regime_) {
			buffer->log_string_stream_ << "Save SWR starting at "
					<< buffer->swrs_[swr_pointer_][0] << " under ID "
					<< swr_pointer_ << " and window number " << swr_win_counter_
					<< " and window center at "
					<< last_pred_pkg_id_ + PRED_WIN / 2 << "\n";
			buffer->Log();
			swr_win_counter_++;

//			double uprior = 1.0 / (NBINSX * NBINSY);
			arma::fmat sub1 = pos_pred_.rows(0, NBINSX / 2 - 1);
			arma::fmat sub2 = pos_pred_.rows(NBINSX / 2, NBINSX - 1);

			double mx = pos_pred_.max();

			unsigned int xm1, ym1, xm2, ym2;
			float mx1 = sub1.max(xm1, ym1);
			float mx2 = sub2.max(xm2, ym2);
			xm2 += NBINSX / 2 + 1;

			long double sm1 = log(arma::sum(arma::sum(arma::exp(sub1 - mx)))) + mx;
			long double sm2 = log(arma::sum(arma::sum(arma::exp(sub2 - mx)))) + mx;

			// UNIFORM P(X) !
			long double logsum = log(arma::sum(arma::sum(arma::exp(pos_pred_ - mx)))) + mx;

			swr_dec_dump_ << swr_pointer_ << " " << last_pred_pkg_id_ + PRED_WIN / 2 << " " << mx1 << " " << mx2 << " " << sm1 << " " << sm2 << " "
					<< BIN_SIZE * (xm1 + 0.5) << " " << BIN_SIZE * (ym1 + 0.5) << " " << BIN_SIZE * (xm2 + 0.5) << " " << BIN_SIZE * (ym2 + 0.5)
					<< " " << logsum <<  "\n";

		} else {
			if (!SWR_SWITCH) {
				pos_pred_.save(pred_dump_pref_ + Utils::Converter::int2str(last_pred_pkg_id_ + PRED_WIN) + ".mat", arma::raw_ascii);
				swr_win_counter_++;
			}
		}
	}
}

void KDClusteringProcessor::validate_prediction_window_bias() {
	// DEBUG
	npred++;
	if (!(npred % 2000)) {
		buffer->log_string_stream_
				<< "Bayesian prediction window bias control: " << npred << " / "
				<< (int) round(
						(last_pred_pkg_id_ - PREDICTION_DELAY)
								/ (float) PRED_WIN) << "\n";
		buffer->Log();
	}
}

void KDClusteringProcessor::process() {
	if (buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER) {
		for (size_t tetr = 0; tetr < tetr_info_->tetrodes_number(); ++tetr) {
			if (!pf_built_[tetr] && !fitting_jobs_running_[tetr] && kde_jobs_running_ < MAX_KDE_JOBS && use_tetrode_[tetr]) {
				buffer->log_string_stream_ << "t " << tetr
						<< ": build kd-tree for tetrode " << tetr << ", "
						<< n_pf_built_ << " / " << tetr_info_->tetrodes_number()
						<< " finished... ";
				kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr],
						total_spikes_[tetr], buffer->feature_space_dims_[tetr]);
				buffer->log_string_stream_ << "done\nt " << tetr << ": cache "
						<< NN_K
						<< " nearest neighbours for each spike in tetrode "
						<< tetr << " (in a separate thread)...\n";
				buffer->Log();

				fitting_jobs_running_[tetr] = true;
				fitting_jobs_[tetr] = new std::thread(
						&KDClusteringProcessor::build_lax_and_tree_separate,
						this, tetr);
				buffer->Log();
			}
		}
	}

	// DEBUG
	if (buffer->spike_buf_pos_unproc_ > 0)
		buffer->CheckPkgIdAndReportTime(buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1]->pkg_id_, "... arrival in KD proc\n");

	// need both speed and PCs
	unsigned int limit =
			LOAD ? std::max<int>(buffer->spike_buf_pos_unproc_, 0) : (
					WAIT_FOR_SPEED_EST ?
							MIN(buffer->spike_buf_pos_speed_,
									(unsigned int )std::max<int>(
											buffer->spike_buf_pos_unproc_ - 1,
											0)) :
							(unsigned int) std::max<int>(
									buffer->spike_buf_pos_unproc_ - 1, 0));
	while (spike_buf_pos_clust_ < limit) {
		Spike *spike = buffer->spike_buffer_[spike_buf_pos_clust_];
		const unsigned int tetr = tetr_info_->Translate(buffer->tetr_info_, (unsigned int) spike->tetrode_);
		const unsigned int nfeat = buffer->feature_space_dims_[tetr];

		// wait until place fields are stabilized
		if (tetr == TetrodesInfo::INVALID_TETRODE
				|| (spike->pkg_id_ < SAMPLING_DELAY && !LOAD) || !use_tetrode_[tetr]) {
			spike_buf_pos_clust_++;
			continue;
		}

		// wait for enough spikes to estimate the firing rate; beware of the rewind after estimating the FRs
		if (!LOAD && (tetrode_sampling_rates_.empty())) {
			if (buffer->fr_estimated_) {
				for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
					unsigned int est_sec_left = (std::min(SAMPLING_END,
							buffer->input_duration_) - SAMPLING_DELAY)
							/ buffer->SAMPLING_RATE;
					std::stringstream ss;
					ss << "Estimated remaining data duration: " << est_sec_left / 60 << " min, " << est_sec_left % 60 << " sec\n";
					unsigned int interleaving_windows_factor = INTERLEAVING_WINDOWS ? 2 : 1;
					tetrode_sampling_rates_.push_back(std::max<int>(0, (int) round( est_sec_left * buffer->fr_estimates_[t] / MIN_SPIKES / interleaving_windows_factor) - 1));
					ss << "\t sampling rate (with speed thold) set to: " << tetrode_sampling_rates_[t] << "\n";
					Log(ss.str());
				}
			} else {
				break;
			}
		}

		// DEBUG
		if (!delay_reached_reported && !LOAD) {
			delay_reached_reported = true;
			Log("Sampling delay over : ", SAMPLING_DELAY);
		}

		if (spike->speed < SPEED_THOLD || spike->discarded_) {
			// DEBUG
//			if (spike->speed < SPEED_THOLD && !spike->discarded_)
//				skipped_spikes_[spike->tetrode_] ++;

			spike_buf_pos_clust_++;
			continue;
		}

		if (!pf_built_[tetr]) {
			// to start prediction only after all PFs are available
			last_pred_pkg_id_ = spike->pkg_id_;

			if ((total_spikes_[tetr] >= MIN_SPIKES && RUN_KDE_ON_MIN_COLLECTED) || buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER || last_pred_pkg_id_ > SAMPLING_END) {
				// build the kd-tree and call kNN for all points, cache indices (unsigned short ??) in the array of pointers to spikes

				if (last_pred_pkg_id_ > SAMPLING_END && !sampling_end_reached_reported_) {
					sampling_end_reached_reported_ = true;
					Log("Sampling end reached : ", SAMPLING_END);
				}

				// start clustering if it is not running yet, otherwise - ignore
				if (!fitting_jobs_running_[tetr] && kde_jobs_running_ < MAX_KDE_JOBS) {
//					build_lax_and_tree_separate(tetr);
					// SHOULD BE DONE IN THE SAME PROCESS, AS KD Search is not parallel
					// dump required data and start process (due to non-thread-safety of ANN)
					// build tree and dump it along with points
					buffer->log_string_stream_ << "t " << tetr << ": build kd-tree for tetrode " << tetr << ", " << n_pf_built_ << " / " << tetr_info_->tetrodes_number() << " finished... ";
					kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], nfeat);
					buffer->log_string_stream_ << "done\nt " << tetr << ": cache " << NN_K << " nearest neighbours for each spike in tetrode " << tetr << " (in a separate thread)...\n";
					buffer->Log();

					Log("Number of KDE jobs before starting a new one: ", kde_jobs_running_);

					fitting_jobs_running_[tetr] = true;
					fitting_jobs_[tetr] = new std::thread( &KDClusteringProcessor::build_lax_and_tree_separate, this, tetr);
				}
			} else {

				if (use_intervals_) {
					// if after current interval -> advance interval pointer
					if (current_interval_ < interval_starts_.size() && spike->pkg_id_ > interval_ends_[current_interval_]) {
						Log("Advance to next interval with spike at ", spike->pkg_id_);
						current_interval_++;
					}

					// out of intervals
					if (current_interval_ >= interval_starts_.size() || spike->pkg_id_ < interval_starts_[current_interval_]) {
						spike_buf_pos_clust_++;
						continue;
					}
				}

				// sample every SAMLING_RATE spikes for KDE estimation
				if (missed_spikes_[tetr] < tetrode_sampling_rates_[tetr]) {
					missed_spikes_[tetr]++;
					spike_buf_pos_clust_++;
					continue;
				}
				missed_spikes_[tetr] = 0;

				if (INTERLEAVING_WINDOWS && ((spike->pkg_id_ / PRED_WIN) % 2 == 0)){
					spike_buf_pos_clust_++;
					continue;
				}

				// copy features and coords to ann_points_int and obs_mats
				for (unsigned int fet = 0; fet < nfeat; ++fet) {
					ann_points_[tetr][total_spikes_[tetr]][fet] = spike->pc[fet];
					obs_mats_[tetr](total_spikes_[tetr], fet) = spike->pc[fet];
				}

				if (!Utils::Math::Isnan(spike->x)) {
					if (BINARY_CLASSIFIER) {
						obs_mats_[tetr](total_spikes_[tetr], nfeat) = spike->x > 140 ? 1.5f : 0.5f;
						obs_mats_[tetr](total_spikes_[tetr], nfeat + 1) = 0.5f;
					} else {
						obs_mats_[tetr](total_spikes_[tetr], nfeat) = spike->x;
						obs_mats_[tetr](total_spikes_[tetr], nfeat + 1) = spike->y;
					}
				} else {
					obs_mats_[tetr](total_spikes_[tetr], nfeat) = (float)buffer->pos_unknown_;
					obs_mats_[tetr](total_spikes_[tetr], nfeat + 1) = (float)buffer->pos_unknown_;
				}

				total_spikes_[tetr]++;
			}

			spike_buf_pos_clust_++;
		} else { // model is ready for decoding
				 // if pf_built but job is designated as running, then it is over and can be joined
			if (fitting_jobs_running_[tetr]) {
				fitting_jobs_running_[tetr] = false;
				fitting_jobs_[tetr]->join();

				if (USE_PRIOR) {
					pos_pred_ = pix_log_;
				}
			}

			// predict from spike in window

			// prediction only after having on all fields
			if (n_pf_built_ < tetr_info_->tetrodes_number()) {
				spike_buf_pos_clust_++;
				continue;
			}

			// prediction only after reaching delay (place field stability, cross-validation etc.)
			if (buffer->spike_buffer_[spike_buf_pos_clust_]->pkg_id_ < PREDICTION_DELAY) {
				spike_buf_pos_clust_++;
				continue;
			} else if (!prediction_delay_reached_reported) {
				buffer->log_string_stream_ << "Prediction delay over (" << PREDICTION_DELAY << ").\n";
				buffer->Log();
				prediction_delay_reached_reported = true;

				last_pred_pkg_id_ = PREDICTION_DELAY;
				buffer->last_preidction_window_ends_[processor_number_] = PREDICTION_DELAY;
			}

			// also rewind SWRs
			while (swr_pointer_ < buffer->swrs_.size() && buffer->swrs_[swr_pointer_][2] < PREDICTION_DELAY) {
				swr_pointer_++;
			}

			// check if NEW swr was detected and has to switch to the SWR regime
			if (SWR_SWITCH) {
				if (!swr_regime_ && swr_pointer_ < buffer->swrs_.size() && buffer->swrs_[swr_pointer_][0] != last_processed_swr_start_) {
					//DEBUG
					buffer->log_string_stream_ << "Switch to SWR prediction regime due to SWR detected at " << buffer->swrs_[swr_pointer_][0] << ", SWR length = "
							<< (buffer->swrs_[swr_pointer_][2] - buffer->swrs_[swr_pointer_][0]) * 1000 / buffer->SAMPLING_RATE << " ms\n";
					buffer->Log();

					swr_regime_ = true;

					PRED_WIN = SWR_PRED_WIN > 0 ? SWR_PRED_WIN : (buffer->swrs_[swr_pointer_][2] - buffer->swrs_[swr_pointer_][0]);

					last_pred_pkg_id_ = buffer->swrs_[swr_pointer_][0];
					last_processed_swr_start_ = buffer->swrs_[swr_pointer_][0];

					// the following is required due to different possible order of SWR detection / spike processing

					// if spikes have been processed before SWR detection - rewind until the first spike in the SW
					if (SINGLE_PRED_PER_SWR){
						// find first spike before the end of SWR
						while (spike->pkg_id_ < buffer->swrs_[swr_pointer_][1]){
							spike_buf_pos_clust_++;
							spike = buffer->spike_buffer_[spike_buf_pos_clust_];
						}
						while (spike->pkg_id_ >= buffer->swrs_[swr_pointer_][1]){
							spike_buf_pos_clust_--;
							spike = buffer->spike_buffer_[spike_buf_pos_clust_];
						}

						// set to prediction_window_spike_number_ spikes back
						spike_buf_pos_clust_ -= std::min(spike_buf_pos_clust_, prediction_window_spike_number_);
					}
					else {
						while (spike_buf_pos_clust_ > 0 && spike->pkg_id_ > last_pred_pkg_id_) {
							spike_buf_pos_clust_--;
							spike = buffer->spike_buffer_[spike_buf_pos_clust_];
						}
					}
					spike_buf_pos_pred_start_ = spike_buf_pos_clust_;

					// !!! if spikes have not been processed yet - rewind until the first spieks in the SWR
					while (spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_ && spike->pkg_id_ < last_pred_pkg_id_) {
						spike_buf_pos_clust_++;
						spike = buffer->spike_buffer_[spike_buf_pos_clust_];
					}

					// reset HMM
					if (USE_HMM) {
						reset_hmm();
					}

					pos_pred_.zeros();
				} // if new SWR detected

				// end SWR regime if the SWR is over or single predicion has been made in the single prediction mode
				if ( swr_regime_ &&
						(last_pred_pkg_id_ > buffer->swrs_[swr_pointer_][2] ||
						(SINGLE_PRED_PER_SWR && last_pred_pkg_id_ > buffer->swrs_[swr_pointer_][0]) )) {
					// DEBUG
					buffer->Log( "Switch to theta prediction regime due to end of SWR at ", (int) buffer->swrs_[swr_pointer_][2]);
					swr_regime_ = false;
					PRED_WIN = THETA_PRED_WIN;

					// reset HMM
					if (USE_HMM) {
						reset_hmm();
					}
					swr_pointer_++;
					swr_win_counter_ = 0;
				}

				// skip all spikes if no swr
				if (!swr_regime_){
					spike_buf_pos_clust_ = limit;
					break;
				}

			} // if SWR_SWITCH

			// at this points all tetrodes have pfs !
			while (spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_) {
				spike = buffer->spike_buffer_[spike_buf_pos_clust_];

				if (spike->pkg_id_ >= last_pred_pkg_id_ + PRED_WIN) {
					break;
				}

				const unsigned int stetr = tetr_info_->Translate(buffer->tetr_info_, spike->tetrode_);

				// in the SWR regime - skip until the first spike in the SWR, don't do this for single prediction per SWR - allow out of range !
				if (stetr == TetrodesInfo::INVALID_TETRODE || spike->discarded_
						|| (swr_regime_ && spike->pkg_id_ < last_processed_swr_start_ && !SINGLE_PRED_PER_SWR) || !use_tetrode_[stetr]) {
					spike_buf_pos_clust_++;
					continue;
				}

				tetr_spiked_[stetr] = true;

				for (unsigned int fet = 0; fet < nfeat; ++fet) {
					pnt_[fet] = spike->pc[fet];
				}

				// PROFILE
//				time_t kds = clock();
				// 5 us for eps = 0.1, 20 ms - for eps = 10.0, prediction quality - ???
				kdtrees_[stetr]->annkSearch(pnt_, neighb_num_, &neighbour_inds_[0], &neighbour_dists_[0], NN_EPS);

				// add 'place field' of the spike with the closest wave shape
				if (neighb_num_ > 1) {
					for (unsigned int i = 0; i < neighb_num_; ++i) {
						window_spikes_[stetr] ++;

//						if (neighbour_dists_[i] > 200){
//							prediction_skipped_spikes_[stetr] ++;
//							continue;
//						}

						pos_pred_ += 1 / float(neighb_num_) * laxs_[stetr][neighbour_inds_[i]];
						if (continuous_prediction_)
							last_spike_fields_.push(1 / float(neighb_num_) * laxs_[stetr][neighbour_inds_[i]]);

					}
				} else {
					pos_pred_ += laxs_[stetr][neighbour_inds_[0]];
					if (continuous_prediction_)
						last_spike_fields_.push(laxs_[stetr][neighbour_inds_[0]]);
				}
//				std::cout << "kd time = " << clock() - kds << "\n";

				if (continuous_prediction_) {
					// subtract first in queue and enqueue current spike
					while (last_spike_fields_.size() > neighb_num_ * prediction_window_spike_number_){
						for (unsigned int n=0; n < neighb_num_; ++n){
							pos_pred_ -= last_spike_fields_.front();
							last_spike_fields_.pop();
						}
					}

					buffer->last_predictions_[processor_number_] = pos_pred_;

					// DEBUG
					buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Prediction ready\n");

				}

				spike_buf_pos_clust_++;
				// spike may be without speed (the last one) - but it's not crucial

				// workaround : if spike number is reached, set PRED_WIN to finish the prediction
				if (prediction_window_spike_number_ > 0 && spike_buf_pos_clust_ - spike_buf_pos_pred_start_ >= prediction_window_spike_number_) {
					PRED_WIN = spike->pkg_id_ - last_pred_pkg_id_;
					// DEBUG
					buffer->log_string_stream_ << "Reached number of spikes of " << prediction_window_spike_number_ << " after " << PRED_WIN * 1000 / buffer->SAMPLING_RATE << " ms from prediction start\n"
							<< "new PRED_WIN = " << PRED_WIN << " (last_pred_pkg_id = " << last_pred_pkg_id_ << ")\n";
					buffer->Log();

					// DEBUG
					buffer->log_string_stream_ << "PRED WIN FROM (SPIKE BUF POS) " << spike_buf_pos_pred_start_ << " TILL " << spike_buf_pos_clust_ << "\n";
					buffer->Log();

					spike_buf_pos_pred_start_ = spike_buf_pos_clust_;
				}
			}

			// if have to wait until the speed estimate - e.g. in case of validation
			if (WAIT_FOR_SPEED_EST &&  buffer->pos_buf_pos_speed_est <= buffer->PositionIndexByPacakgeId(last_pred_pkg_id_)){
				return;
			}

			// if prediction is final and end of window has been reached (last spike is beyond the window)
			// 		or prediction will be finalized in subsequent iterations
			if (spike->pkg_id_ >= last_pred_pkg_id_ + PRED_WIN) {

//				std::stringstream ss;
//				ss << "WINDOW OVER AT " <<  last_pred_pkg_id_ + PRED_WIN << " with spike pkg id " << spike->pkg_id_ << ", finalize prediction\n";
//				Log(ss.str());

				// DEBUG
				buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "...  arrival in KD at the prediction start\n");

				if (!continuous_prediction_ && !IGNORE_LX) {
					// edges of the window
					// account for the increase in the firing rate during high synchrony with additional factor
					const float DE_SEC = float((prediction_window_spike_number_ > 0) ? (THETA_PRED_WIN / (float) buffer->SAMPLING_RATE ) :( PRED_WIN / (float) buffer->SAMPLING_RATE * (swr_regime_ ? SWR_COMPRESSION_FACTOR : 1.0)));

					for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
						// TODO ? subtract even if did not spike
//						double skipped_spikes_factor = (window_spikes_[t] - prediction_skipped_spikes_[t]) / double(window_spikes_[t]);
						if (tetr_spiked_[t]) {
							pos_pred_ -= DE_SEC * lxs_[t]; // * skipped_spikes_factor;
						}
					}
				}

				// DUMP decoded coordinate
				unsigned int mx = 0, my = 0;
				pos_pred_.max(mx, my);

				// !!! NORM TO SUM UP TO 1 !!!
//				pos_pred_ = arma::exp(pos_pred_);
//				pos_pred_ /= arma::sum(arma::sum(pos_pred_));
//				pos_pred_ = arma::log(pos_pred_);

				dump_positoins_if_needed(mx, my);
				dump_swr_window_spike_count();

				last_pred_probs_ = pos_pred_;

				dump_prediction_if_needed();

				// THE POINT AT WHICH THE PREDICTION IS READY
				// DEBUG
				buffer->CheckPkgIdAndReportTime(spike->pkg_id_, " ... prediction for the window ready\n");

				// updated in HMM
				buffer->last_predictions_[processor_number_] = pos_pred_;
				buffer->last_preidction_window_ends_[processor_number_] = last_pred_pkg_id_ + PRED_WIN;

				last_pred_pkg_id_ += PRED_WIN;

				// re-init prediction variables
				tetr_spiked_ = std::vector<bool>(tetr_info_->tetrodes_number(), false);

				if (USE_HMM)
					update_hmm_prediction();

				validate_prediction_window_bias();

				// TODO ? WHY for every tetrode ?
				if (!continuous_prediction_)
					pos_pred_ = USE_PRIOR ? ((float)tetr_info_->tetrodes_number() * pix_log_) : arma::fmat(NBINSX, NBINSY, arma::fill::zeros);

				// return to display prediction etc...
				//		(don't need more spikes at this stage)

				if (prediction_window_spike_number_ > 0) {
					// ??? already done above
					spike_buf_pos_pred_start_ = spike_buf_pos_clust_;
					// TODO what should be here
					// was: (SWR_PRED_WIN > 0 ? SWR_PRED_WIN : (buffer->swrs_[swr_pointer_][2] - buffer->swrs_[swr_pointer_][0]))
					// should not limit becase of potential unlimited rewinds

					// should not be limited ? in both swr / non-swr regime
					PRED_WIN = 24000000;//swr_regime_ ? 24000000 : THETA_PRED_WIN;
				}

				// rewind back to get predictions of the overlapping windows
				if (swr_regime_ && prediction_windows_overlap_ > 0 && (spike_buf_pos_clust_ >= prediction_windows_overlap_ + buffer->SPIKE_BUF_HEAD_LEN)) {
					spike_buf_pos_clust_ -= prediction_windows_overlap_;
					// find first good spike
					while ((spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_) && buffer->spike_buffer_[spike_buf_pos_clust_]->discarded_)
						spike_buf_pos_clust_++;

					last_pred_pkg_id_ = buffer->spike_buffer_[spike_buf_pos_clust_]->pkg_id_;

					spike_buf_pos_pred_start_ = spike_buf_pos_clust_;

					// DEBUG
					Log("Rewind to get overlapping windows until position: ", spike_buf_pos_clust_);
					Log("	Update last_pred_pkg_id : ", buffer->spike_buffer_[spike_buf_pos_clust_]->pkg_id_);
					Log("	Update spike_buf_pos_pred_start_ : ", spike_buf_pos_pred_start_);
				}

				for (unsigned int t=0; t < tetr_info_->tetrodes_number(); ++t){
					prediction_skipped_spikes_[t] = 0;
					window_spikes_[t] = 0;
				}

				return;
			}
		}
	}

	// WORKAROUND
	if (processor_number_ == 0) {
		buffer->spike_buf_pos_clust_ = spike_buf_pos_clust_;
	}
}

void KDClusteringProcessor::build_lax_and_tree_separate(
		const unsigned int tetr) {

	kde_mutex_.lock();
	kde_jobs_running_ ++;
	kde_mutex_.unlock();

	if (total_spikes_[tetr] == 0) {
		Log("ERROR: No spikes collected for KDE. Exiting. Tetrode = ", tetr);
		// because this is the child thread
		exit(564879);
	}

	// create pos_buf and dump (first count points)
	unsigned int npoints = 0;
	arma::Mat<float> pos_buf(2, buffer->pos_buf_pos_);
	unsigned int pos_interval = 0;
	int nskip = 0;
	for (unsigned int n = 0; n < buffer->pos_buf_pos_; ++n) {
		const SpatialInfo & si = buffer->positions_buf_[n];

		// if pos is unknown or speed is below the threshold - ignore
		if (Utils::Math::Isnan(si.x_pos()) || si.pkg_id_ < SAMPLING_DELAY || si.pkg_id_ > SAMPLING_END) {
			continue;
		}

		if (si.speed_ < SPEED_THOLD) { // || Utils::Math::Isnan(si.dirvar_) || si.dirvar_ > 0.5) {
			nskip++;
			continue;
		}

		// skip if out of the intervals
		if (use_intervals_) {
			unsigned int pos_time = buffer->PacakgeIdByPositionIndex(n);
			while (pos_interval < interval_starts_.size() && pos_time > interval_ends_[pos_interval]) {
				pos_interval++;
			}

			if (pos_interval >= interval_starts_.size()) {
				continue;
			}

			if (pos_time < interval_starts_[pos_interval]) {
				continue;
			}
		}

		if (BINARY_CLASSIFIER) {
			// TODO !! parametrize environment threshold
			pos_buf(0, npoints) = si.x_pos() > 140 ? 1.5f : 0.5f;
			pos_buf(1, npoints) = 0.5f;
		} else {
			pos_buf(0, npoints) = si.x_pos();
			pos_buf(1, npoints) = si.y_pos();
		}
		npoints++;
	}

	// OOR check
	unsigned int nout = 0;
	for (unsigned int i=0; i < pos_buf.n_cols; ++i){
		if (pos_buf(0, i) > NBINSX * BIN_SIZE || pos_buf(1, i) > NBINSY * BIN_SIZE){
			nout ++;
		}
	}
	if (nout / double(pos_buf.n_cols) > POS_OOR_LIMIT){
		Log("ERROR: limit of positoins out of range exceeded, check NBINS X / NBINS Y / BIN SIZE!");
		buffer->processing_over_ = true;
		exit(235325);
	}

	// DEBUG
	Log("Skipped due to speed: ", nskip);
	Log("Tetrode: ", tetr);
//	Log("Skipped due to speed according to counter: ", skipped_spikes_[tetr]);

	if (npoints < MIN_POS_SAMPLES) {
		Log("ERROR: number of position samples == 0");
		exit(91824);
	}

	Log("Number of pos samples: ", npoints);

	// lock while writing to avoid errors
	kde_mutex_.lock();
	std::ofstream kdstream(BASE_PATH + Utils::Converter::int2str(tetr) + ".kdtree");
	kdtrees_[tetr]->Dump(ANNtrue, kdstream);
	kdstream.close();

	// dump obs_mat
	obs_mats_[tetr].resize(total_spikes_[tetr], obs_mats_[tetr].n_cols);
	obs_mats_[tetr].save(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_obs.mat", arma::raw_ascii);

	// free resources
	obs_mats_[tetr].clear();
	delete kdtrees_[tetr];

	pos_buf = pos_buf.cols(0, npoints - 1);
	pos_buf.save(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_pos_buf.mat", arma::raw_ascii);
	kde_mutex_.unlock();

	unsigned int last_pkg_id = buffer->last_pkg_id;
	// if using intervals, provide sum of interval lengths until last_pkg_id
	if (use_intervals_) {
		last_pkg_id = 0;
		for (unsigned int i = 0; i < current_interval_; ++i) {
			last_pkg_id += interval_ends_[i] - interval_starts_[i];
		}
		if (current_interval_ < interval_starts_.size()) // && buffer->last_pkg_id < interval_ends_[current_interval_])
			last_pkg_id += buffer->last_pkg_id - interval_starts_[current_interval_];

		Log("Due to interval usage, calling KDE with ", (int) last_pkg_id);
		Log("	while the real last_pkg_id is ", (int) buffer->last_pkg_id);
	}

	// find last pkg id for each tetrode
	unsigned int tspikepos = buffer->spike_buf_pos - 1;
	while (tspikepos > 0 && (buffer->spike_buffer_[tspikepos] == nullptr || buffer->spike_buffer_[tspikepos]->tetrode_ != (int) tetr)) {
		tspikepos--;
	}
	if (tspikepos > 0 && buffer->spike_buffer_[tspikepos] != nullptr)
		last_pkg_id = buffer->spike_buffer_[tspikepos]->pkg_id_;

	// change in calculated firing rate due to speed-filtered out spikes : <predicted rate based on not speed-filtered spikes> / <actual rate in the sampling period>
	double effective_rate_factor = buffer->fr_estimates_[tetr] * (last_pkg_id - SAMPLING_DELAY) / double( buffer->SAMPLING_RATE * total_spikes_[tetr] * (tetrode_sampling_rates_[tetr] + 1));
	Log("Effective rate factor: ", effective_rate_factor);

	// build commandline to start kde_estimator
	std::ostringstream os;
	const unsigned int nfeat = buffer->feature_space_dims_[tetr];
	os << kde_path_ << " " << tetr << " " << nfeat << " " << NN_K << " " << NN_K_COORDS << " " << nfeat << " " << MULT_INT << " " << NBINSX << " " << NBINSY << " " << total_spikes_[tetr] << " " << buffer->SAMPLING_RATE << " " << last_pkg_id << " "
			<< SAMPLING_DELAY << " " << effective_rate_factor * (tetrode_sampling_rates_[tetr] + 1) << " " << BIN_SIZE << " " << NN_EPS << " " << SIGMA_X << " " << SIGMA_A << " " << SIGMA_XX << " " << SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD << " "
			<< SPIKE_GRAPH_COVER_NNEIGHB << " " << KD_MIN_OCC << " " << BASE_PATH;
	buffer->log_string_stream_ << "t " << tetr << ": Start external kde_estimator with command (tetrode, dim, nn_k, nn_k_coords, n_feat, mult_int,  bin_size, n_bins. min_spikes, sampling_rate, buffer_sampling_rate, last_pkg_id, sampling_delay, nn_eps, sigma_x, sigma_a, sigma_xx, vc_dist_thold, vc_nneighb, kd_min_occ)\n\t" << os.str() << "\n";
	buffer->Log();

	int retval = system(os.str().c_str());
	if (retval < 0) {
		buffer->Log("ERROR: impossible to start kde_estimator!");
	} else {
		buffer->log_string_stream_ << "t " << tetr << ": kde estimator exited with code " << retval << "\n";
		buffer->Log();
	}

	pf_built_[tetr] = true;

	kde_mutex_.lock();
	n_pf_built_++;
	kde_jobs_running_ --;
	kde_mutex_.unlock();

	if (n_pf_built_ == tetr_info_->tetrodes_number()) {
		Log("KDE at all tetrodes done, exiting...\n");
		buffer->processing_over_ = true;
	} else {
		Log("Number of tetrodes built: ", n_pf_built_);
	}
}

void KDClusteringProcessor::JoinKDETasks() {
	for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
		if (fitting_jobs_running_[t])
			fitting_jobs_[t]->join();
	}
	buffer->Log("All KDE jobs joined...");
}

std::string KDClusteringProcessor::name() {
	return "KD Clustering";
}
