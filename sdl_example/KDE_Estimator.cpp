/*
 * KDE_Estimator.cpp
 *
 *  Created on: Jun 26, 2014
 *      Author: igor
 */
#include <armadillo>
#include <ANN/ANN.h>

#include <fstream>
#include <iostream>
#include <vector>

int DIM;
int NN_K;
int NN_K_COORDS;
int N_FEAT;
int MULT_INT;
int BIN_SIZE;
int NN_EPS;
int NBINS;

int MIN_SPIKES;
int SAMPLING_RATE;
int BUFFER_SAMPLING_RATE;
int BUFFER_LAST_PKG_ID;

int tetr;

std::string BASE_PATH;
bool SAVE = true;

ANNkd_tree *kdtree_, *kdtree_coords;

arma::Mat<int> spike_coords_int, coords_normalized, pos_buf;
arma::mat px, lx, pix, pix_log, obs_mat, lax;

int **ann_points_int;

std::vector<ANNidx*> knn_cache;

ANNpointArray ann_points_;
ANNpointArray ann_points_coords;

const int total_spikes_;

long long kern_H_ax_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr, const int& x, const int& y) {
	// TODO: implement efficiently (integer with high precision)
	// TODO: check for the overlow (MULT order X DIM X MAX(coord order / feature sorder) X squared = (10 + 1 + 2) * 2 = 26 < 32 bit )

	long long sum = 0;
//	sum += (obs_spikes_[tetr][spikei1]->x - obs_spikes_[tetr][spikei2]->x) / X_STD;
//	sum += (obs_spikes_[tetr][spikei1]->y - obs_spikes_[tetr][spikei2]->y) / Y_STD;

	int *pcoord1 = ann_points_int[spikei1], *pcoord2 = ann_points_int[spikei2];
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

void build_pax_(const unsigned int tetr, const unsigned int spikei, const arma::mat& occupancy) {
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
				double spikex = obs_mat(knn_cache[spikei][ni], N_FEAT);
				if (abs(spikex - 1023) < 1){
					continue;
				}

				long long logprob = kern_H_ax_(spikei, knn_cache[spikei][ni], tetr, coords_normalized(xb, 0), coords_normalized(yb, 1));

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

	if (SAVE){
		std::string save_path = BASE_PATH + Utils::NUMBERS[tetr] + "_" + Utils::Converter::int2str((int)spikei) + ".mat";
		pf.save(save_path, arma::raw_ascii);
//		std::cout << save_path << "\n";
	}

	lax[spikei] = pf;
}

int main(int argc, char **argv){
	// read the following variables and perform KDE estimation and tree building

	std::cout << "build kd-tree for tetrode " << tetr;
	kdtree_ = new ANNkd_tree(ann_points_, total_spikes_, DIM);
	std::cout << "done\n Cache " << NN_K << " nearest neighbours for each spike...\n";

	if (SAVE){
		std::ofstream kdtree_stream(BASE_PATH + Utils::NUMBERS[tetr] + "_kdtree.mat");
		kdtree_->Dump(ANNtrue, kdtree_stream);
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
		float stdf = arma::stddev(obs_mat.col(f));
		std::cout << "std of feature " << f << " = " << stdf << "\n";
		stds.push_back(stdf);
		avg_feat_std += stdf;
	}
	avg_feat_std /= N_FEAT;
	float stdx = arma::stddev(obs_mat.col(N_FEAT));
	float stdy = arma::stddev(obs_mat.col(N_FEAT + 1));
	std::cout << "std of x  = " << stdx << "\n";
	std::cout << "std of y  = " << stdy << "\n";
	// normalize coords to have the average feature std
	for (int s = 0; s < total_spikes_[tetr]; ++s) {
		// ... loss of precision 1) from rounding to int; 2) by dividing int on float
		spike_coords_int(s, 0) = (int)(obs_mat(s, N_FEAT) * avg_feat_std * MULT_INT / stdx);  //= stdx / avg_feat_std;
		spike_coords_int(s, 1) = (int)(obs_mat(s, N_FEAT + 1) * avg_feat_std * MULT_INT / stdy);  //= stdy / avg_feat_std;

		// points to build coords 2d-tree, raw x and y coords
		ann_points_coords[s][0] = obs_mat(s, N_FEAT);
		ann_points_coords[s][1] = obs_mat(s, N_FEAT + 1);
	}

	// build 2d-tree for coords
	kdtree_coords = new ANNkd_tree(ann_points_coords, total_spikes_[tetr], 2);
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
			kdtree_coords->annkSearch(pnt, NN_K_COORDS, nnIdx_coord, dists_coord, NN_EPS);

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

			px(xb, yb) = kde_sum;
		}
	}
	if (SAVE){
		px.save(BASE_PATH + Utils::NUMBERS[tetr] + "px.mat", arma::raw_ascii);
	}

	// compute occupancy KDE - pi(x) from tracking position sampling
	// TODO 2d-tree ? how many neighbours needed ?
	// overall tetrode average firing rate
	double mu = MIN_SPIKES * SAMPLING_RATE * BUFFER_SAMPLING_RATE / BUFFER_LAST_PKG_ID;
	std::cout << "Average firing rate on tetrode: " << mu << "\n";
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			double xc = BIN_SIZE * (0.5 + xb);
			double yc = BIN_SIZE * (0.5 + yb);

			double kde_sum = 0;
			unsigned int npoints = pos_buf.n_cols;
			for (int n = 0; n < pos_buf.n_cols; ++n) {
				double sum = 0;

				double xdiff = xc - pos_buf(0, n);
				double ydiff = yc - pos_buf(1, n);

				sum += xdiff * xdiff / stdx;
				sum += ydiff * ydiff / stdy;

				kde_sum += exp(- sum / 2);
			}

			kde_sum /= npoints;

			pix_log(xb, yb) = log(kde_sum);
			pix(xb, yb) = kde_sum;
		}
	}

	// compute generalized rate function lambda(x)
	double pisum = arma::sum(arma::sum(pix));
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			// absolute value of this function matter, but constant near p(x) and pi(x) is the same (as the same kernel K_H_x is used)
			if (pix_log(xb, yb) > 0.001 * pisum){
				lx(xb, yb) = mu * px(xb, yb) / pix(xb, yb);
			}
		}
	}

	if (SAVE){
		pix_log.save(BASE_PATH + "pix_log.mat", arma::raw_ascii);
		pix.save(BASE_PATH + "pix.mat", arma::raw_ascii);
		lx.save(BASE_PATH + Utils::NUMBERS[tetr] + "lx.mat", arma::raw_ascii);
	}

	// pre-compute matrix of normalized bin centers
	// ASSUMING xbin size == ybin size
	for (int xb = 0; xb < NBINS; ++xb) {
		coords_normalized(xb, 0) = (int)((float)BIN_SIZE * (0.5 + xb) * avg_feat_std * MULT_INT / stdx);
		coords_normalized(xb, 1) = (int)((float)BIN_SIZE * (0.5 + xb) * avg_feat_std * MULT_INT / stdy);
	}

	// cache k nearest neighbours for each spike (for KDE computation)
	ANNdist *dists = new ANNdist[NN_K];
	// call kNN for each point
	for (int p = 0; p < total_spikes_[tetr]; ++p) {
		ANNidx *nnIdx = new ANNidx[NN_K];

		kdtree_->annkSearch(ann_points_[p], NN_K, nnIdx, dists, NN_EPS);
		// TODO: cast/copy to unsigned short array?
		knn_cache.push_back(nnIdx);

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

		build_pax_(tetr, p, pix);
	}

	// if save - concatenate all matrices laxs_[tetr] and save (along with individual, for fast visualization)
	if (SAVE){
		arma::mat laxs_tetr_(NBINS, NBINS * MIN_SPIKES);
		for (int s = 0; s < MIN_SPIKES; ++s) {
			laxs_tetr_.cols(s*NBINS, (s + 1) * NBINS - 1) = lax[s];
		}
		laxs_tetr_.save(BASE_PATH + Utils::NUMBERS[tetr] + "_tetr.mat");
	}

	return 0;
}
