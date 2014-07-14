/*
 * KDE_Estimator.cpp
 *
 *  Created on: Jun 26, 2014
 *      Author: igor
 */
#include <armadillo>
#include <ANN/ANN.h>

#include <math.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "LFPBuffer.h"

int DIM;
int NN_K;
int NN_K_COORDS;
int N_FEAT;
int MULT_INT;
int MULT_INT_FEAT;
int BIN_SIZE;
int NBINS;

int MIN_SPIKES;
int SAMPLING_RATE;
int BUFFER_SAMPLING_RATE;
int BUFFER_LAST_PKG_ID;
int SAMPLING_DELAY;

double NN_EPS;

int tetr;

std::string BASE_PATH;
bool SAVE = true;

// first loaded, 2nd - created
ANNkd_tree *kdtree_, *kdtree_coords;

//spike_coords_int - computed; coords_normalized - computed; pos buf - copied from buffer->pos_buf and loaded
arma::Mat<int> spike_coords_int, coords_normalized, pos_buf;
// this one is loaded
arma::mat obs_mat;
// these are created
arma::mat px(NBINS, NBINS, arma::fill::zeros), lx(NBINS, NBINS, arma::fill::zeros),
		pix(NBINS, NBINS, arma::fill::zeros), pix_log(NBINS, NBINS, arma::fill::zeros);

std::vector<arma::mat> lax;

int **ann_points_int;

std::vector<ANNidx*> knn_cache;

ANNpointArray ann_points_;
ANNpointArray ann_points_coords;

// TODO init
int total_spikes;

// compute value of joint distribution kernel for spike2 centered in spike1 with NORMALIZED (and converted to integer with high precision) coordinates x and y
long long kern_H_ax_(const unsigned int spikei1, const unsigned int spikei2, const int& x, const int& y) {
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

	// TODO !!! parametrize from nbins and bin size
	const double MIN_OCC = 0.0001;

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

				// TODO: optimze kernel computation and don't compute (a - a_i) each time
				long long logprob = kern_H_ax_(spikei, knn_cache[spikei][ni], coords_normalized(xb, 0), coords_normalized(yb, 1));

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
			if (occupancy(xb, yb)/occ_sum >= MIN_OCC){
				pf(xb, yb) = log(kern_sum / nspikes / occupancy(xb, yb));
			}
		}
	}

	double occ_min = pf.min();
	// set min at low occupancy
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			if (occupancy(xb, yb)/occ_sum < MIN_OCC){
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

//	std::cout << "build kd-tree for tetrode " << tetr;
//	kdtree_ = new ANNkd_tree(ann_points_, total_spikes_, DIM);
//	std::cout << "done\n Cache " << NN_K << " nearest neighbours for each spike...\n";

	if (argc != 17){
		std::cout << "Exactly 17 parameters should be provided (starting with tetrode, ending with BASE_PATH)!";
		exit(1);
	}

	int *pars[] = {&tetr, &DIM, &NN_K, &NN_K_COORDS, &N_FEAT, &MULT_INT, &MULT_INT_FEAT, &BIN_SIZE, &NBINS, &MIN_SPIKES, &SAMPLING_RATE, &BUFFER_SAMPLING_RATE, &BUFFER_LAST_PKG_ID, &SAMPLING_DELAY};
	for(int p=0; p < 14; ++p){
		*(pars[p]) = atoi(argv[p+1]);
	}
	NN_EPS = atof(argv[15]);
	BASE_PATH = argv[16];

	std::cout << "t " << tetr << ": " << "start KDE estimation\n";

	// load trees, extract points, load mats
	std::ifstream kdstream(BASE_PATH + Utils::Converter::int2str(tetr) + ".kdtree");
	kdtree_ = new ANNkd_tree(kdstream);
	kdstream.close();
	ann_points_ = kdtree_->thePoints();

	obs_mat.load(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_obs.mat");
	pos_buf.load(BASE_PATH  + "tmp_" + Utils::NUMBERS[tetr] + "_pos_buf.mat");

	// DEBUG
	std::cout << "t " << tetr << ": obs " << obs_mat.n_rows << " X " << obs_mat.n_cols << "\n";
	std::cout << "t " << tetr << ": pos " << pos_buf.n_rows << " X " << pos_buf.n_cols << "\n";

	total_spikes = obs_mat.n_rows;

	// allocate estimation matrices
	px = arma::mat(NBINS, NBINS, arma::fill::zeros);
	lx = arma::mat(NBINS, NBINS, arma::fill::zeros);
	pix = arma::mat(NBINS, NBINS, arma::fill::zeros);
	pix_log = arma::mat(NBINS, NBINS, arma::fill::zeros);
	lax.resize(MIN_SPIKES,  arma::mat(NBINS, NBINS, arma::fill::zeros));

	ann_points_coords = annAllocPts(MIN_SPIKES, 2);
	spike_coords_int = arma::Mat<int>(total_spikes, 2);
	coords_normalized = arma::Mat<int>(NBINS, 2);
	ann_points_int = new int*[MIN_SPIKES];
	for (int d = 0; d < MIN_SPIKES; ++d) {
		ann_points_int[d] = new int[DIM];
	}

	// ---
	// NORMALIZE STDS
	// TODO: !!! KDE / kd-tree search should be performed with the same std normalization !!!
	// current: don't normalize feature covariances (as clustering is done in this way) but normalize x/y std to have the average feature std
	std::vector<float> stds;
	float avg_feat_std = .0f;
	for (int f = 0; f < N_FEAT; ++f) {
		float stdf = arma::stddev(obs_mat.col(f));
		std::cout << "t " << tetr << ": std of feature " << f << " = " << stdf << "\n";
		stds.push_back(stdf);
		avg_feat_std += stdf;
	}
	avg_feat_std /= N_FEAT;
	float stdx = arma::stddev(obs_mat.col(N_FEAT));
	float stdy = arma::stddev(obs_mat.col(N_FEAT + 1));
	std::cout << "t " << tetr << ": " << "std of x  = " << stdx << "\n";
	std::cout << "t " << tetr << ": " << "std of y  = " << stdy << "\n";
	// normalize coords to have the average feature std
	for (int s = 0; s < total_spikes; ++s) {
		// ... loss of precision 1) from rounding to int; 2) by dividing int on float
		spike_coords_int(s, 0) = (int)(obs_mat(s, N_FEAT) * avg_feat_std * MULT_INT / stdx);  //= stdx / avg_feat_std;
		spike_coords_int(s, 1) = (int)(obs_mat(s, N_FEAT + 1) * avg_feat_std * MULT_INT / stdy);  //= stdy / avg_feat_std;

		// points to build coords 2d-tree, raw x and y coords
		ann_points_coords[s][0] = obs_mat(s, N_FEAT);
		ann_points_coords[s][1] = obs_mat(s, N_FEAT + 1);

		for (int f = 0; f < N_FEAT; ++f) {
			ann_points_int[s][f] = (int)round(obs_mat(s, f) * MULT_INT_FEAT);
		}
	}

	// build 2d-tree for coords
	kdtree_coords = new ANNkd_tree(ann_points_coords, total_spikes, 2);
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
		px.save(BASE_PATH + Utils::NUMBERS[tetr] + "_px.mat", arma::raw_ascii);
	}

	// compute occupancy KDE - pi(x) from tracking position sampling
	// TODO 2d-tree ? how many neighbours needed ?
	// overall tetrode average firing rate, spikes / s
	double mu = MIN_SPIKES * SAMPLING_RATE * BUFFER_SAMPLING_RATE / (BUFFER_LAST_PKG_ID - SAMPLING_DELAY);
	std::cout << "t " << tetr << ": Average firing rate on tetrode: " << mu << "\n";
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
	std::cout << "t " << tetr << ": done pix\n";

	// compute generalized rate function lambda(x)
	// TODO !!! uniform + parametrize
	const double MIN_OCC = 0.0001;
	double pisum = arma::sum(arma::sum(pix));
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			// absolute value of this function matter, but constant near p(x) and pi(x) is the same (as the same kernel K_H_x is used)
			if (pix(xb, yb) > MIN_OCC * pisum){
				lx(xb, yb) = mu * px(xb, yb) / pix(xb, yb);
			}
		}
	}

	if (SAVE){
		pix_log.save(BASE_PATH + "pix_log.mat", arma::raw_ascii);
		pix.save(BASE_PATH + "pix.mat", arma::raw_ascii);
		lx.save(BASE_PATH + Utils::NUMBERS[tetr] + "_lx.mat", arma::raw_ascii);
	}

	// pre-compute matrix of normalized bin centers
	// ASSUMING xbin size == ybin size
	for (int xb = 0; xb < NBINS; ++xb) {
		coords_normalized(xb, 0) = (int)((float)BIN_SIZE * (0.5 + xb) * avg_feat_std * MULT_INT / stdx);
		coords_normalized(xb, 1) = (int)((float)BIN_SIZE * (0.5 + xb) * avg_feat_std * MULT_INT / stdy);
	}
	std::cout << "t " << tetr << ": done coords_normalized\n";

	// cache k nearest neighbours for each spike (for KDE computation)
	ANNdist *dists = new ANNdist[NN_K];
	// call kNN for each point
	for (int p = 0; p < total_spikes; ++p) {
		ANNidx *nnIdx = new ANNidx[NN_K];

		kdtree_->annkSearch(ann_points_[p], NN_K, nnIdx, dists, NN_EPS);
		// TODO: cast/copy to unsigned short array?
		knn_cache.push_back(nnIdx);

		// DEBUG
		//					Utils::Output::printIntArray(nnIdx, 10);
	}
	delete dists;
	std::cout << "t " << tetr << ": done neighbours caching\n";

	// compute p(a_i, x) for all spikes (as KDE of nearest neighbours neighbours)
	time_t start = clock();
	//const arma::mat& occupancy = pfProc_->GetSmoothedOccupancy();
	for (int p = 0; p < total_spikes; ++p) {
		// DEBUG
		if (!(p % 5000)){
			std::cout.precision(2);
			std::cout << "t " << tetr << ": " << p << " place fields built, last 5000 in " << (clock() - start)/ (float)CLOCKS_PER_SEC << " sec....\n";
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

	std::cout << "t " << tetr << ": FINISHED\n";

	return 0;
}
