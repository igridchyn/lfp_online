/*
 * KDClusteringProcessor.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#include "KDClusteringProcessor.h"
#include <fstream>

KDClusteringProcessor::KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes, const std::string base_path, PlaceFieldProcessor* pfProc)
	: LFPProcessor(buf)
	, MIN_SPIKES(num_spikes)
	//, BASE_PATH("/hd1/data/bindata/jc103/jc84/jc84-1910-0116/pf_ws/pf_"){
	, BASE_PATH(base_path)
	, pfProc_(pfProc){
	// TODO Auto-generated constructor stub

	const unsigned int tetrn = buf->tetr_info_->tetrodes_number;

	kdtrees_.resize(tetrn);
	ann_points_.resize(tetrn);
	total_spikes_.resize(tetrn);
	obs_spikes_.resize(tetrn);
	knn_cache_.resize(tetrn);
	spike_place_fields_.resize(tetrn);
	ann_points_int_.resize(tetrn);
	spike_coords_int_.resize(tetrn);
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

		spike_coords_int_[t] = arma::Mat<int>(MIN_SPIKES, 2, arma::fill::zeros);

		// tmp
		obs_mats_[t] = arma::mat(MIN_SPIKES, 14);

		// bin / (x,y)
		coords_normalized_[t] = arma::Mat<int>(NBINS, 2, arma::fill::zeros);

		laxs_[t].resize(MIN_SPIKES, arma::mat(NBINS, NBINS, arma::fill::zeros));
	}

	pf_built_.resize(tetrn);

	pix_log_ = arma::mat(NBINS, NBINS, arma::fill::zeros);
	pix_ = arma::mat(NBINS, NBINS, arma::fill::zeros);

	if (LOAD){
		// load occupancy
		pix_.load(BASE_PATH + "pix.mat");
		pix_log_.load(BASE_PATH + "pix_log.mat");

		for (int t = 0; t < tetrn; ++t) {
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
			laxs_tetr_.save(BASE_PATH + Utils::NUMBERS[t] + "_tetr.mat");

			// load marginal rate function
			lxs_[t].load(BASE_PATH + Utils::NUMBERS[t] + "lx.mat");
			pxs_[t].load(BASE_PATH + Utils::NUMBERS[t] + "px.mat");

			pf_built_[t] = true;

			std::ifstream kdtree_stream(BASE_PATH + Utils::NUMBERS[t] + "_kdtree.mat");
			kdtrees_[t] = new ANNkd_tree(kdtree_stream);
			kdtree_stream.close();
		}

		n_pf_built_ = tetrn;
	}
}

KDClusteringProcessor::~KDClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

// bulid p(a,x) with a of a given spike and (x,y)  coordinates of centers of time bins (normalized)
void KDClusteringProcessor::build_pax_(const unsigned int tetr, const unsigned int spikei, const arma::mat& occupancy) {
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

				long long logprob = kern_H_ax_(spikei, knn_cache_[tetr][spikei][ni], tetr, coords_normalized_[tetr](xb, 0), coords_normalized_[tetr](yb, 1));

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
long long inline KDClusteringProcessor::kern_H_ax_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr, const int& x, const int& y) {
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

	int neighbx = spike_coords_int_[tetr](spikei2, 0);
	int xdiff = (neighbx - x);
	sum += xdiff * xdiff;
	// Y coordinate
	int neighby = spike_coords_int_[tetr](spikei2, 1);
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

void KDClusteringProcessor::process(){
	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
		const unsigned int tetr = spike->tetrode_;

		if (!pf_built_[tetr]){
			if (total_spikes_[tetr] >= MIN_SPIKES){
				// build the kd-tree and call kNN for all points, cache indices (unsigned short ??) in the array of pointers to spikes

				// start clustering if it is not running yet, otherwise - ignore
				if (!fitting_jobs_running_[tetr]){
					fitting_jobs_running_[tetr] = true;
					fitting_jobs_[tetr] = new std::thread(&KDClusteringProcessor::build_lax_and_tree, this, tetr);

					// !!! WORKAROUND due to thread-unsafety of ANN
					fitting_jobs_[tetr]->join();
					fitting_jobs_running_[tetr] = false;
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
						ann_points_int_[tetr][total_spikes_[tetr]][chan * 3 + pc] = (int)round(spike->pc[chan][pc] * MULT_INT_FEAT);

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
		}
		else{
			if (fitting_jobs_running_[tetr]){
				fitting_jobs_running_[tetr] = false;
				fitting_jobs_[tetr]->join();
			}

			// predict from spike in window

			// edges of the window
			const double DE_SEC = 100 / 1000.0;
			const unsigned int right_edge = buffer->spike_buffer_[buffer->spike_buf_pos_unproc_ - 1]->pkg_id_;
			const unsigned int left_edge  = right_edge - 100 * buffer->SAMPLING_RATE / 1000;

			// sparse prediction + prediction only after having on all fields
			if (right_edge < 10000 || n_pf_built_ < buffer->tetr_info_->tetrodes_number || right_edge - last_pred_pkg_id_ < PRED_RATE){
				buffer->spike_buf_pos_clust_ ++;
				continue;
			}
			last_pred_pkg_id_ = right_edge;

			// posterior position probabilities map
			// log of prior = pi(x)
			arma::mat pos_pred_(pix_log_);

			// should be added for each tetrode on which spikes occurred
//			pos_pred_ -= DE_SEC  * lxs_[tetr];
			std::vector<bool> tetr_spiked(buffer->tetr_info_->tetrodes_number, false);

			unsigned int spike_ind = buffer->spike_buf_pos_unproc_ - 1;

			Spike *spike = buffer->spike_buffer_[spike_ind];
			ANNpoint pnt = annAllocPt(DIM);
			double dist;
			int closest_ind;
			while(spike->pkg_id_ > left_edge){
				const unsigned int stetr = spike->tetrode_;
				tetr_spiked[stetr] = true;

				// TODO ? wait until all place fields are constructed ?
				if (!pf_built_[stetr]){
					spike_ind --;
					spike = buffer->spike_buffer_[spike_ind];
					continue;
				}

				// TODO: convert PC in spike to linear array
				for (int pc=0; pc < 3; ++pc) {
					// TODO: tetrode channels
					for(int chan=0; chan < 4; ++chan){
						pnt[chan * 3 + pc] = spike->pc[chan][pc];
					}
				}

				kdtrees_[stetr]->annkSearch(pnt, 1, &closest_ind, &dist, NN_EPS);

				pos_pred_ += laxs_[stetr][closest_ind];

				spike_ind --;
				spike = buffer->spike_buffer_[spike_ind];
			}

			for (int t = 0; t < buffer->tetr_info_->tetrodes_number; ++t) {
				if (tetr_spiked[t]){
					pos_pred_ -= DE_SEC  * lxs_[t];
				}
			}

			last_pred_probs_ = pos_pred_;
			double minval = arma::min(arma::min(pos_pred_));
//			pos_pred_ = pos_pred_ - minval;
			pos_pred_ = arma::exp(pos_pred_ / 100);
			buffer->last_prediction_ = pos_pred_.t();

			// DEBUG
			if (!(last_pred_pkg_id_ % 200)){
				pos_pred_.save(BASE_PATH + "tmp_pred_" + Utils::Converter::int2str(last_pred_pkg_id_) + ".mat", arma::raw_ascii);
				std::cout << "save prediction...\n";
			}
		}

		buffer->spike_buf_pos_clust_ ++;
	}
}

void KDClusteringProcessor::build_lax_and_tree_separate(const unsigned int tetr) {
	// dump required data and start process (due to non-thread-safety of ANN)
	std::cout << "build kd-tree for tetrode " << tetr << ", " << n_pf_built_ << " / " << buffer->tetr_info_->tetrodes_number << " finished... ";
	kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], DIM);
	std::cout << "done\n Cache " << NN_K << " nearest neighbours for each spike (in a separate thread)...\n";


}

void KDClusteringProcessor::build_lax_and_tree(const unsigned int tetr) {
	std::cout << "build kd-tree for tetrode " << tetr << ", " << n_pf_built_ << " / " << buffer->tetr_info_->tetrodes_number << " finished... ";
	kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], DIM);
	std::cout << "done\n Cache " << NN_K << " nearest neighbours for each spike...\n";

	ANNpointArray ann_points_coords = annAllocPts(MIN_SPIKES, 2);

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
		spike_coords_int_[tetr](s, 0) = (int)(obs_mats_[tetr](s, N_FEAT) * avg_feat_std * MULT_INT / stdx);  //= stdx / avg_feat_std;
		spike_coords_int_[tetr](s, 1) = (int)(obs_mats_[tetr](s, N_FEAT + 1) * avg_feat_std * MULT_INT / stdy);  //= stdy / avg_feat_std;

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
		pxs_[tetr].save(BASE_PATH + Utils::NUMBERS[tetr] + "px.mat", arma::raw_ascii);
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
			if (pix_log_(xb, yb) > 0.001 * pisum){
				lxs_[tetr](xb, yb) = mu * pxs_[tetr](xb, yb) / pix_(xb, yb);
			}
		}
	}

	if (SAVE){
		pix_log_.save(BASE_PATH + "pix_log.mat", arma::raw_ascii);
		pix_.save(BASE_PATH + "pix.mat", arma::raw_ascii);
		lxs_[tetr].save(BASE_PATH + Utils::NUMBERS[tetr] + "lx.mat", arma::raw_ascii);
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

		build_pax_(tetr, p, pix_);
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
}

void KDClusteringProcessor::JoinKDETasks(){
    for(int t=0; t < buffer->tetr_info_->tetrodes_number; ++t){
        if(fitting_jobs_running_[t])
            fitting_jobs_[t]->join();
    }
    std::cout << "All KDE jobs joined...\n";
}
