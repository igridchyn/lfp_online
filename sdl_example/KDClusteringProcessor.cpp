/*
 * KDClusteringProcessor.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#include "KDClusteringProcessor.h"

KDClusteringProcessor::KDClusteringProcessor(LFPBuffer *buf, const unsigned int num_spikes, const std::string base_path)
	: LFPProcessor(buf)
	, MIN_SPIKES(num_spikes)
	//, BASE_PATH("/hd1/data/bindata/jc103/jc84/jc84-1910-0116/pf_ws/pf_"){
	, BASE_PATH(base_path){
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
	}

	pf_built_.resize(tetrn);
}

KDClusteringProcessor::~KDClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

// bulid p(a,x) with a of a given spike and (x,y)  coordinates of centers of time bins (normalized)
void KDClusteringProcessor::build_pax_(const unsigned int tetr, const unsigned int spikei) {
	arma::mat pf(NBINS, NBINS, arma::fill::zeros);

	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			// order of >= 30
			long long kern_sum = 0;

			unsigned int nspikes = 0;
			// compute KDE (proportional to log-probability) over cached nearest neighbours
			for (int ni = 0; ni < NN_K; ++ni) {
				Spike *spike = obs_spikes_[tetr][knn_cache_[tetr][spikei][ni]];
				if (spike->x == 1023){
					continue;
				}
				nspikes++;

				int logprob = kern_(spikei, knn_cache_[tetr][spikei][ni], tetr, coords_normalized_[tetr](xb, 0), coords_normalized_[tetr](yb, 1));
				kern_sum += logprob;
			}

			// scaled by MULT_INT ^ 2
			pf(xb, yb) = (double)kern_sum / (MULT_INT * MULT_INT) / nspikes;
		}
	}

	spike_place_fields_[tetr][spikei] = pf;

	if (SAVE){
		std::string save_path = BASE_PATH + Utils::NUMBERS[tetr] + "_" + Utils::Converter::int2str((int)spikei) + ".mat";
		pf.save(save_path, arma::raw_ascii);
//		std::cout << save_path << "\n";
	}
}

// x/y instead of coords of the second spike
int inline KDClusteringProcessor::kern_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr, const int& x, const int& y) {
	// TODO: implement efficiently (integer with high precision)
	// TODO: check for the overlow (MULT order X DIM X MAX(coord order / feature sorder) X squared = (10 + 1 + 2) * 2 = 26 < 32 bit )

	int sum = 0;
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
	return - sum;
}

void KDClusteringProcessor::process(){
	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
		const unsigned int tetr = spike->tetrode_;

		if (!pf_built_[tetr]){
			if (total_spikes_[tetr] >= MIN_SPIKES){
				// build the kd-tree and call kNN for all points, cache indices (unsigned short ??) in the array of pointers to spikes
				std::cout << "build kd-tree for tetrode " << tetr << ", " << n_pf_built_ << " / " << buffer->tetr_info_->tetrodes_number << " finished... ";
				kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], DIM);
				std::cout << "done\n Cache " << NN_K << " nearest neighbours for each spike...\n";

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
				}
				delete dists;
				std::cout << "done\n";

				// compute p(a_i, x) for all spikes (as KDE of nearest neighbours neighbours)
				time_t start = clock();
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

					build_pax_(tetr, p);
				}

				pf_built_[tetr] = true;
				n_pf_built_ ++;
			}
			else{
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
		}
		else{
			// predict from spike in window
		}

		buffer->spike_buf_pos_clust_ ++;
	}
}
