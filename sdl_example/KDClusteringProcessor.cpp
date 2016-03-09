/*
 * KDClusteringProcessor.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
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
	}
	//laxs_tetr_.save(BASE_PATH + Utils::NUMBERS[t] + "_tetr.mat");

	// load marginal rate function
	std::string lxs_path = BASE_PATH + Utils::NUMBERS[t] + "_lx.mat";
	Utils::FS::CheckFileExistsWithError(lxs_path, (Utils::Logger*) this);
	lxs_[t].load(lxs_path);
	double maxval = arma::max(arma::max(lxs_[t]));
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
				buf->config_->getInt("kd.pred.win", 2000)), SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD(
				buf->config_->getFloat(
						"kd.spike.graph.cover.distance.threshold", 0)), SPIKE_GRAPH_COVER_NNEIGHB(
				buf->config_->getInt("kd.spike.graph.cover.nneighb", 1)), POS_SAMPLING_RATE(
				buf->config_->getFloat("pos.sampling.rate", 512.0)), FR_ESTIMATE_DELAY(
				buf->config_->getFloat("kd.frest.delay", 1000000)), DUMP_SPEED_THOLD(
				buf->config_->getFloat("kd.dump.speed.thold", .0)), WAIT_FOR_SPEED_EST(
				getBool("kd.wait.speed")), RUN_KDE_ON_MIN_COLLECTED(
				getBool("kd.run.on.min")), kde_path_(
				buf->config_->getString("kd.path", "./kde_estimator")), continuous_prediction_(
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
				buf->config_->getInt("kd.max.jobs", 5)), sr_path_(
				buf->config_->getString("out.path.base") + buf->config_->getString("kd.sr.path", "sampling_rates.txt")), sr_save_(
				buf->config_->getBool("kd.sr.save", false)), sr_load_(
				buf->config_->getBool("kd.sr.load", false))
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

	Log("Allocate distributino matrices");
	laxs_.resize(tetrn);
	pxs_.resize(tetrn, arma::fmat(NBINSX, NBINSY, arma::fill::zeros));
	lxs_.resize(tetrn, arma::fmat(NBINSX, NBINSY, arma::fill::zeros));

	unsigned int max_dim = 0;

	Log("Allocate observation arrays");
	for (unsigned int t = 0; t < tetrn; ++t) {
		const unsigned int dim = buf->feature_space_dims_[t];

		ann_points_[t] = annAllocPts(MIN_SPIKES * 2, dim);
		spike_place_fields_[t].reserve(MIN_SPIKES * 2);

		// tmp
		obs_mats_[t] = arma::fmat(MIN_SPIKES * 2,
				buffer->feature_space_dims_[t] + 2);

		if (buffer->feature_space_dims_[t] > max_dim)
			max_dim = buffer->feature_space_dims_[t];
	}

	pf_built_.resize(tetrn);

	pix_log_ = arma::fmat(NBINSX, NBINSY, arma::fill::zeros);

	if (LOAD) {
		// load occupancy
		Utils::FS::CheckFileExistsWithError(BASE_PATH + "pix_log.mat",
				(Utils::Logger*) this);
		pix_log_.load(BASE_PATH + "pix_log.mat");

		std::vector<std::thread*> load_threads;
		for (unsigned int t = 0; t < tetrn; ++t) {
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

	// TODO !!! priors
	reset_hmm();

	hmm_traj_.resize(NBINSX * NBINSY);

	// save params only for model building scenario
	if (SAVE) {
		std::string parpath = BASE_PATH + "params.txt";
		std::ofstream fparams(parpath);
		fparams
				<< "SIGMA_X, SIGMA_A, SIGMA_XX, MULT_INT, SAMPLING_RATE, NN_K, NN_K_SPACE(obsolete), MIN_SPIKES, SAMPLING_RATE, SAMPLING_DELAY, NBINSX, NBINSY, BIN_SIZE, SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD, SPIKE_GRAPH_COVER_NNEIGHB, NN_EPS\n"
				<< SIGMA_X << " " << SIGMA_A << " " << SIGMA_XX << " "
				<< MULT_INT << " " << SAMPLING_RATE << " " << NN_K << " "
				<< NN_K_COORDS << " " << MIN_SPIKES << " " << SAMPLING_RATE
				<< " " << SAMPLING_DELAY << " " << NBINSX << " " << NBINSY
				<< " " << BIN_SIZE << " "
				<< SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD << " "
				<< SPIKE_GRAPH_COVER_NNEIGHB << " " << NN_EPS << "\n";
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
	window_spike_counts_.open("../out/window_spike_counts.txt");

	pnt_ = annAllocPt(max_dim);

	last_spike_pkg_ids_by_tetrode_.resize(tetr_info_->tetrodes_number(), 0);

	Log("Construction ready");

	// DEBUG
//	skipped_spikes_.resize(tetr_info_->tetrodes_number(), 0);
//	debug_.open("debug.txt");

	neighbour_dists_.resize(neighb_num_);
	neighbour_inds_.resize(neighb_num_);

	if (sr_load_){
		std::ifstream fsrs(sr_path_);
		for (unsigned int t=0; t < tetr_info_->tetrodes_number(); ++t){
			unsigned int sr;
			fsrs >> sr;
			tetrode_sampling_rates_.push_back(sr);
		}

		Log("Loaded sampling rates");
	}
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

			for (unsigned int x = MAX(0, xb - HMM_NEIGHB_RAD);
					x <= MIN(xb + HMM_NEIGHB_RAD, NBINSX - 1); ++x) {
				for (unsigned int y = MAX(0, yb - HMM_NEIGHB_RAD);
						y <= MIN(yb + HMM_NEIGHB_RAD, NBINSY - 1); ++y) {
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
	int ind = (int) round(
			(last_pred_pkg_id_ - PRED_WIN / 2) / (float) POS_SAMPLING_RATE);
	float corrx = buffer->positions_buf_[ind].x_pos();
	float corry = buffer->positions_buf_[ind].y_pos();

	// for consistency of comparison
	if (last_pred_pkg_id_ > DUMP_DELAY) {
		// STATS - dump best HMM trajectory by backtracking
		std::ofstream dec_hmm(
				std::string("dec_hmm_") + Utils::NUMBERS[processor_number_]
						+ ".txt");
		int t = hmm_traj_[0].size() - 1;
		// best last x,y
		unsigned int x, y;
		hmm_prediction_.max(x, y);
		while (t >= 0) {
			dec_hmm << BIN_SIZE * (x + 0.5) << " " << BIN_SIZE * (y + 0.5)
					<< " ";
			int posind = (int) ((t * PRED_WIN + PREDICTION_DELAY)
					/ (float) POS_SAMPLING_RATE);
			corrx = buffer->positions_buf_[posind].x_pos();
			corry = buffer->positions_buf_[posind].y_pos();
			dec_hmm << corrx << " " << corry << "\n";
			unsigned int b = hmm_traj_[y * NBINSX + x][t];

			if (b < NBINSX * NBINSY) {
				y = b / NBINSX;
				x = b % NBINSX;
			} else {
				buffer->log_string_stream_ << "Trajectory is screwed up at "
						<< t << ", keep the previous position\n";
				buffer->Log();
			}

			t--;
		}
		dec_hmm.flush();

		buffer->Log("Exit after dumping the HMM prediction :",
				(int) DUMP_DELAY);
		exit(0);
	}

	buffer->last_predictions_[processor_number_] = arma::exp(
			hmm_prediction_ / display_scale_);
}

void KDClusteringProcessor::reset_hmm() {
	hmm_prediction_ = arma::fmat(NBINSX, NBINSY, arma::fill::zeros);
	if (USE_PRIOR) {
		hmm_prediction_ = pix_log_;
	}
}

void KDClusteringProcessor::dump_positoins_if_needed(const unsigned int& mx,
		const unsigned int& my) {
	if (last_pred_pkg_id_ > DUMP_DELAY && last_pred_pkg_id_ < DUMP_END) {
		// DEBUG
		if (!dump_delay_reach_reported_) {
			dump_delay_reach_reported_ = true;
			Log("Dump delay reached: ", (int) DUMP_DELAY);
		}
		if (last_pred_pkg_id_ + PRED_WIN >= DUMP_END
				&& !dump_end_reach_reported_) {
			dump_end_reach_reported_ = true;
			Log("Dump end reached: ", (int) DUMP_END);
			if (DUMP_END_EXIT) {
				Log("EXIT upon dump end reached");
				exit(0);
			}
		}

		unsigned int gtx = buffer->pos_unknown_, gty = buffer->pos_unknown_;

		// !!! WORKAROUND - WOULDN'T WORK WITH THE REWIND
		SpatialInfo &pose =
				buffer->positions_buf_[(unsigned int) (last_pred_pkg_id_
						/ (float) POS_SAMPLING_RATE)];
		gtx = pose.x_pos();
		gty = pose.y_pos();

		// for output need non-nan value
		if (Utils::Math::isnan(gtx)) {
			gtx = buffer->pos_unknown_;
			gty = buffer->pos_unknown_;
		}

		if (pose.speed_ >= DUMP_SPEED_THOLD) {
			double mult = BINARY_CLASSIFIER ? 100 : 1.0;
			dec_bayesian_ << BIN_SIZE * (mx + 0.5) * mult << " "
					<< BIN_SIZE * (my + 0.5) * mult << " " << gtx << " " << gty
					<< "\n";
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

void KDClusteringProcessor::dump_prediction_if_needed(
		const arma::fmat& pos_pred) {
	// DEBUG save prediction
	if (pred_dump_) {
		if (swr_regime_) {
			buffer->log_string_stream_ << "Save SWR starting at "
					<< buffer->swrs_[swr_pointer_][0] << " under ID "
					<< swr_pointer_ << " and window number " << swr_win_counter_
					<< " and window center at "
					<< last_pred_pkg_id_ + PRED_WIN / 2 << "\n";
			buffer->Log();
			pos_pred_.save(
					pred_dump_pref_ + "swr_"
							+ Utils::Converter::int2str(swr_pointer_) + "_"
							+ Utils::Converter::int2str(swr_win_counter_) + "_"
							+ Utils::Converter::int2str(
									last_pred_pkg_id_ + PRED_WIN / 2) + "_"
							+ Utils::Converter::int2str(processor_number_)
							+ ".mat", arma::raw_ascii);
			swr_win_counter_++;
		} else {
			if (!SWR_SWITCH) {
				pos_pred_.save(
						pred_dump_pref_
								+ Utils::Converter::int2str(
										last_pred_pkg_id_ + PRED_WIN) + ".mat",
						arma::raw_ascii);
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
			if (!pf_built_[tetr] && !fitting_jobs_running_[tetr] && kde_jobs_running_ < MAX_KDE_JOBS) {
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
		buffer->CheckPkgIdAndReportTime(
				buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1]->pkg_id_,
				"Time from after package extraction until arrival in KD proc\n");

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
		const unsigned int tetr = tetr_info_->Translate(buffer->tetr_info_,
				(unsigned int) spike->tetrode_);
		const unsigned int nfeat = buffer->feature_space_dims_[tetr];

		// wait until place fields are stabilized
		if (tetr == TetrodesInfo::INVALID_TETRODE
				|| (spike->pkg_id_ < SAMPLING_DELAY && !LOAD)) {
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
					tetrode_sampling_rates_.push_back(
							std::max<int>(0, (int) round( est_sec_left * buffer->fr_estimates_[t] / MIN_SPIKES) - 1));
					ss << "\t sampling rate (with speed thold) set to: " << tetrode_sampling_rates_[t] << "\n";
					Log(ss.str());
				}

				// write sampling rates to file
				if (sr_save_){
					std::ofstream fsampling_rates(sr_path_);
					for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
						fsampling_rates << tetrode_sampling_rates_[t] << "\n";
					}
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
					if (current_interval_ >= interval_starts_.size()) {
						spike_buf_pos_clust_++;
						continue;
					}

					if (spike->pkg_id_ < interval_starts_[current_interval_]) {
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

				// copy features and coords to ann_points_int and obs_mats
				for (unsigned int fet = 0; fet < nfeat; ++fet) {
					ann_points_[tetr][total_spikes_[tetr]][fet] = spike->pc[fet];
					obs_mats_[tetr](total_spikes_[tetr], fet) = spike->pc[fet];
				}

				if (!Utils::Math::isnan(spike->x)) {
					if (BINARY_CLASSIFIER) {
						obs_mats_[tetr](total_spikes_[tetr], nfeat) = spike->x > 140 ? 1.5 : 0.5;
						obs_mats_[tetr](total_spikes_[tetr], nfeat + 1) = 0.5;
					} else {
						obs_mats_[tetr](total_spikes_[tetr], nfeat) = spike->x;
						obs_mats_[tetr](total_spikes_[tetr], nfeat + 1) = spike->y;
					}
				} else {
					obs_mats_[tetr](total_spikes_[tetr], nfeat) = buffer->pos_unknown_;
					obs_mats_[tetr](total_spikes_[tetr], nfeat + 1) = buffer->pos_unknown_;
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
					while (spike_buf_pos_clust_ > 0 && spike != nullptr && spike->pkg_id_ > last_pred_pkg_id_) {
						spike_buf_pos_clust_--;
						spike = buffer->spike_buffer_[spike_buf_pos_clust_];
					}
					spike_buf_pos_pred_start_ = spike_buf_pos_clust_;

					// !!! if spikes have not been processed yet - rewind until the first spieks in the SWR
					while (spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_ && spike != nullptr && spike->pkg_id_ < last_pred_pkg_id_) {
						spike_buf_pos_clust_++;
						spike = buffer->spike_buffer_[spike_buf_pos_clust_];
					}

					// reset HMM
					if (USE_HMM) {
						reset_hmm();
					}

					// for continuous prediction - adjustment be the generalized firing rate
					for (unsigned int t = 0; t < tetr_info_->tetrodes_number();
							++t) {
						last_spike_pkg_ids_by_tetrode_[t] =
								buffer->swrs_[swr_pointer_][0];
					}

					pos_pred_.zeros();
				}

				// end SWR regime if the SWR is over
				if (swr_regime_ && last_pred_pkg_id_ > buffer->swrs_[swr_pointer_][2]) {
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
			}

			// at this points all tetrodes have pfs !
			while (spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_) {
				spike = buffer->spike_buffer_[spike_buf_pos_clust_];

				if (spike->pkg_id_ >= last_pred_pkg_id_ + PRED_WIN) {
					break;
				}

				const unsigned int stetr = tetr_info_->Translate(buffer->tetr_info_, spike->tetrode_);

				if (stetr == TetrodesInfo::INVALID_TETRODE || spike->discarded_) {
					spike_buf_pos_clust_++;
					continue;
				}

				// in the SWR regime - skip until the first spike in the SWR
				if (swr_regime_ && spike->pkg_id_ < last_processed_swr_start_) {
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
						pos_pred_ += 1 / float(neighb_num_) * laxs_[stetr][neighbour_inds_[i]];
					}
				} else {
					pos_pred_ += laxs_[stetr][neighbour_inds_[0]];
				}
//				std::cout << "kd time = " << clock() - kds << "\n";

				// TODO: from window start if the spike was first
				if (continuous_prediction_) {
					// from last spike at the current tetrode
					const double DE_SEC = (spike->pkg_id_ - last_spike_pkg_ids_by_tetrode_[stetr]) / (float) buffer->SAMPLING_RATE * (swr_regime_ ? SWR_COMPRESSION_FACTOR : 1.0);
					// !!! TODO !!!
					pos_pred_ -= DE_SEC * lxs_[stetr];

					// TODO !!! make a reference for speed
					buffer->last_predictions_[processor_number_] = pos_pred_;
//					buffer->last_predictions_[processor_number_] = arma::exp(pos_pred_ / display_scale_);

					// DEBUG
					buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Prediction ready\n");
				}

				last_spike_pkg_ids_by_tetrode_[stetr] = spike->pkg_id_;

				spike_buf_pos_clust_++;
				// spike may be without speed (the last one) - but it's not crucial

				// workaround : if spike number is reached, set PRED_WIN to finish the prediction
				if (prediction_window_spike_number_ > 0 && spike_buf_pos_clust_ - spike_buf_pos_pred_start_ >= prediction_window_spike_number_) {
					PRED_WIN = spike->pkg_id_ - last_pred_pkg_id_;
					// DEBUG
					buffer->log_string_stream_ << "Reached number of spikes of " << prediction_window_spike_number_ << " after " << PRED_WIN * 1000 / buffer->SAMPLING_RATE << " ms from prediction start\n"
							<< "new PRED_WIN = " << PRED_WIN << " (last_pred_pkg_id = " << last_pred_pkg_id_ << ")\n";
					buffer->Log();
					spike_buf_pos_pred_start_ = spike_buf_pos_clust_;
				}
			}

			// if have to wait until the speed estimate
			if (WAIT_FOR_SPEED_EST && (unsigned int) (last_pred_pkg_id_ / (float) POS_SAMPLING_RATE) >= buffer->pos_buf_pos_speed_est) {
				return;
			}

			// if prediction is final and end of window has been reached (last spike is beyond the window)
			// 		or prediction will be finalized in subsequent iterations
			if (spike->pkg_id_ >= last_pred_pkg_id_ + PRED_WIN) {

				// DEBUG
				buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Time from after package extraction until arrival in KD at the prediction start\n");

				// edges of the window
				// account for the increase in the firing rate during high synchrony with additional factor
				const double DE_SEC = PRED_WIN / (float) buffer->SAMPLING_RATE * (swr_regime_ ? SWR_COMPRESSION_FACTOR : 1.0);

				if (!continuous_prediction_) {
					for (size_t t = 0; t < tetr_info_->tetrodes_number(); ++t) {
						// TODO ? subtract even if did not spike
						if (tetr_spiked_[t]) {
							pos_pred_ -= DE_SEC * lxs_[t];
						}
					}
				}

				// DUMP decoded coordinate
				unsigned int mx = 0, my = 0;
				pos_pred_.max(mx, my);

				dump_positoins_if_needed(mx, my);
				dump_swr_window_spike_count();

				last_pred_probs_ = pos_pred_;
				// to avoid OOR
				double minpred = arma::max(arma::max(last_pred_probs_));
				last_pred_probs_ -= minpred;

				dump_prediction_if_needed(pos_pred_);

				// THE POINT AT WHICH THE PREDICTION IS READY
				// DEBUG
				buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Time from after package extraction until prediction for the window ready\n");

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
				pos_pred_ = USE_PRIOR ? (tetr_info_->tetrodes_number() * pix_log_) : arma::fmat(NBINSX, NBINSY, arma::fill::zeros);

				// return to display prediction etc...
				//		(don't need more spikes at this stage)

				if (prediction_window_spike_number_ > 0) {
					spike_buf_pos_pred_start_ = spike_buf_pos_clust_;
					// TODO what should be here
					// was: (SWR_PRED_WIN > 0 ? SWR_PRED_WIN : (buffer->swrs_[swr_pointer_][2] - buffer->swrs_[swr_pointer_][0]))
					// should not limit becase of potential unlimited rewinds
					PRED_WIN = swr_regime_ ? 24000000 : THETA_PRED_WIN;
				}

				// rewind back to get predictions of the overlapping windows
				if (swr_regime_ && prediction_windows_overlap_ > 0 && (spike_buf_pos_clust_ >= prediction_windows_overlap_ + buffer->SPIKE_BUF_HEAD_LEN)) {
					spike_buf_pos_clust_ -= prediction_windows_overlap_;
					// find first good spike
					while ((spike_buf_pos_clust_ < buffer->spike_buf_pos_unproc_) && (buffer->spike_buffer_[spike_buf_pos_clust_]->discarded_ || !buffer->spike_buffer_[spike_buf_pos_clust_]->aligned_))
						spike_buf_pos_clust_++;

					last_pred_pkg_id_ = buffer->spike_buffer_[spike_buf_pos_clust_]->pkg_id_;

					spike_buf_pos_pred_start_ = spike_buf_pos_clust_;

					// DEBUG
					Log("Rewind to get overlapping windows until position: ", spike_buf_pos_clust_);
					Log("	Update last_pred_pkg_id : ", buffer->spike_buffer_[spike_buf_pos_clust_]->pkg_id_);
					Log("	Update spike_buf_pos_pred_start_ : ", spike_buf_pos_pred_start_);
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

	std::ofstream kdstream(BASE_PATH + Utils::Converter::int2str(tetr) + ".kdtree");
	kdtrees_[tetr]->Dump(ANNtrue, kdstream);
	kdstream.close();

	// dump obs_mat
	obs_mats_[tetr].resize(total_spikes_[tetr], obs_mats_[tetr].n_cols);
	obs_mats_[tetr].save(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_obs.mat", arma::raw_ascii);

	// free resources
	obs_mats_[tetr].clear();
	delete kdtrees_[tetr];

	// create pos_buf and dump (first count points)
	unsigned int npoints = 0;
	arma::Mat<float> pos_buf(2, buffer->pos_buf_pos_);
	unsigned int pos_interval = 0;
	int nskip = 0;
	for (unsigned int n = 0; n < buffer->pos_buf_pos_; ++n) {
		if (buffer->positions_buf_[n].speed_ < SPEED_THOLD) {
			nskip++;
		}

		// if pos is unknown or speed is below the threshold - ignore
		if (Utils::Math::isnan(buffer->positions_buf_[n].x_pos()) || buffer->positions_buf_[n].speed_ < SPEED_THOLD) {
			continue;
		}

		// skip if out of the intervals
		if (use_intervals_) {
			// TODO account for buffer rewinds
			unsigned int pos_time = POS_SAMPLING_RATE * n;
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
			pos_buf(0, npoints) = buffer->positions_buf_[n].x_pos() > 140 ? 1.5 : 0.5;
			pos_buf(1, npoints) = 0.5;
		} else {
			pos_buf(0, npoints) = buffer->positions_buf_[n].x_pos();
			pos_buf(1, npoints) = buffer->positions_buf_[n].y_pos();
		}
		npoints++;
	}

	// DEBUG
	Log("Skipped due to speed: ", nskip);
	Log("Tetrode: ", tetr);
//	Log("Skipped due to speed according to counter: ", skipped_spikes_[tetr]);

	// TODO: MINIMAL POSITION SAMPLES
	if (npoints == 0) {
		Log("ERROR: number of position samples == 0");
		exit(91824);
	}

	Log("Number of pos samples: ", npoints);

	pos_buf = pos_buf.cols(0, npoints - 1);
	pos_buf.save(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_pos_buf.mat", arma::raw_ascii);

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
			<< SPIKE_GRAPH_COVER_NNEIGHB << " " << BASE_PATH;
	buffer->log_string_stream_ << "t " << tetr << ": Start external kde_estimator with command (tetrode, dim, nn_k, nn_k_coords, n_feat, mult_int,  bin_size, n_bins. min_spikes, sampling_rate, buffer_sampling_rate, last_pkg_id, sampling_delay, nn_eps, sigma_x, sigma_a, sigma_xx, vc_dist_thold, vc_nneighb)\n\t" << os.str() << "\n";
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
		exit(0);
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
