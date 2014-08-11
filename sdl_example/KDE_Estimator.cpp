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
int BIN_SIZE;
int NBINS;

const unsigned int BLOCK_SIZE = 3;
int NBLOCKS;

int MIN_SPIKES;
int SAMPLING_RATE;
int BUFFER_SAMPLING_RATE;
int BUFFER_LAST_PKG_ID;
int SAMPLING_DELAY;

double NN_EPS;
double SIGMA_X;
double SIGMA_A;
double SIGMA_XX;

int tetr;

std::string BASE_PATH;
bool SAVE = true;

// first loaded, 2nd - created
ANNkd_tree *kdtree_, *kdtree_ax_, *kd_tree_coords_;
// neighbours caches: spike locations for each bin and most similar spikes for each spike
std::vector<int *> cache_xy_bin_spikes_;
std::vector<int *> cache_sim_spikes_;
// also, cache distances [conv. to float if too much memory is used ?]
std::vector<double *> cache_xy_bin_spikes_dists_;
std::vector<double *> cache_sim_spikes_dists_;

// cache of NNs for bin blocks [9 X 9]
std::vector<int *> cache_block_neighbs_;

//spike_coords_int - computed; coords_normalized - computed; pos buf - copied from buffer->pos_buf and loaded
arma::Mat<int> spike_coords_int, coords_normalized, pos_buf;
// this one is loaded
arma::mat obs_mat;
// these are created
arma::mat px(NBINS, NBINS, arma::fill::zeros), lx(NBINS, NBINS, arma::fill::zeros),
		pix(NBINS, NBINS, arma::fill::zeros), pix_log(NBINS, NBINS, arma::fill::zeros);

std::vector<arma::mat> lax;

int **ann_points_int;

ANNpointArray ann_points_;
ANNpointArray ax_points_;
ANNpointArray ann_points_coords;

// TODO init
int total_spikes;

void cache_spike_and_bin_neighbours(){

	// CACHE spikes with closest WS
	// number of neihgbours to be cached - both for spikes and bin centers
	const unsigned int NN_CACHE = 1000;
	// cache spike neighbours
	ANNpointArray ann_points_ws = kdtree_->thePoints();
	for (int s = 0; s < total_spikes; ++s) {
		// TODO alloc mat ?
		ANNidx *nn_idx = new ANNidx[NN_CACHE];
		ANNdist *dd = new ANNdist[NN_CACHE];
		// TODO relax EPS ?
		kdtree_->annkSearch(ann_points_ws[s], NN_CACHE, nn_idx, dd, NN_EPS);
		cache_sim_spikes_.push_back(nn_idx);
		cache_sim_spikes_dists_.push_back(dd);
	}

	// build kd-tree in the (x, y) space
	kd_tree_coords_ = new ANNkd_tree(ann_points_coords, total_spikes, 2);
	// cache neighbours for each bin center
	for (int xb = 0; xb < NBINS; ++xb) {
			for (int yb = 0; yb < NBINS; ++yb) {
				ANNpoint xy_pnt = annAllocPt(2);
				xy_pnt[0] = BIN_SIZE * (xb + 0.5);
				xy_pnt[1] = BIN_SIZE * (yb + 0.5);
				ANNidx *nn_idx = new ANNidx[NN_CACHE];
				ANNdist *dd = new ANNdist[NN_CACHE];
				// TODO relax eps ? ()
				kd_tree_coords_->annkSearch(xy_pnt, NN_CACHE, nn_idx, dd, NN_EPS);
				cache_xy_bin_spikes_.push_back(nn_idx);
				cache_xy_bin_spikes_dists_.push_back(dd);
			}
	}
	std::cout << "Done (x_b, y_b) neighbours and distances caching\n";
}

// compute value of joint distribution kernel for spike2 centered in spike1 with NORMALIZED (and converted to integer with high precision) coordinates x and y
long long kern_H_ax_(const unsigned int& spikei2, const int& x, const int& y) {
	// TODO: check for the overlow (MULT order X DIM X MAX(coord order / feature sorder) X squared = (10 + 1 + 2) * 2 = 26 < 32 bit )

	long long sum = 0;

	// coords are already normalized to have the same variance as features (average)
	// X coordinate
	int neighbx = spike_coords_int(spikei2, 0);
	int xdiff = (neighbx - x);
	sum += xdiff * xdiff;
	// Y coordinate
	int neighby = spike_coords_int(spikei2, 1);
	int ydiff = (neighby - y);
	sum += ydiff * ydiff;

	// order : ~ 10^9 for 12 features scaled by 2^10 = 10^3
	return - sum / 2;
}

void build_pax_(const unsigned int& tetr, const unsigned int& spikei, const arma::mat& occupancy) {
	arma::mat pf(NBINS, NBINS, arma::fill::zeros);

	const double occ_sum = arma::sum(arma::sum(occupancy));

	// TODO !!! parametrize from nbins and bin size
	const double MIN_OCC = 0.0001;

	// pre-compute feature part of the sum (same for all coordinates)
	std::vector<long long> feature_sum;
	for (int ni = 0; ni < total_spikes; ++ni) {
		long long sum = 0;
		int *pcoord1 = ann_points_int[spikei], *pcoord2 = ann_points_int[ni];
		for (int d = 0; d < DIM; ++d, ++pcoord1, ++pcoord2) {
			int coord1 = *pcoord1, coord2 = *pcoord2;
			sum += (coord1 - coord2) * (coord1 - coord2);
		}
		feature_sum.push_back(- sum / 2);
	}

	// TODO: cache MIN_SPIKES X NBINS X NBINS distances btw spike loc and bin centers [already computed at p(x) computation]

	ANNpoint p_bin_spike = ax_points_[spikei];
	// to restore later
	double x_s = p_bin_spike[N_FEAT];
	double y_s = p_bin_spike[N_FEAT + 1];

	// TODO don't allocate each time
	ANNidx *nn_idx = new ANNidx[NN_K];
	ANNdist *dd = new ANNdist[NN_K];

	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			// order of >= 30
			double kern_sum = 0;

			unsigned int nspikes = 0;
			// compute KDE (proportional to log-probability) over cached nearest neighbours
			// TODO ? exclude self from neighbours list ?

			// do kd search if first bin in the block, otherwise - use cached neighbours
			// find closest points for 'spike with a given sha'
			int nblock = (yb / BLOCK_SIZE) * NBLOCKS + xb / BLOCK_SIZE;
			if (!(xb % BLOCK_SIZE) && !(yb % BLOCK_SIZE)){
				// form a point (a_i, x_b, y_b) and find its nearest neighbours in (a, x) space for KDE
				// have to choose central bin of the block
				p_bin_spike[N_FEAT] = coords_normalized(xb < NBINS - 1 ? xb + 1 : xb, 0);
				p_bin_spike[N_FEAT + 1] = coords_normalized(yb < NBINS - 1 ? yb + 1 : yb, 1);

				kdtree_ax_->annkSearch(p_bin_spike, NN_K, cache_block_neighbs_[nblock], dd, NN_EPS / 3.0);
			}
			nn_idx = cache_block_neighbs_[nblock];


			// to rough estimation
//			int k_in = kdtree_ax_->annkFRSearch(p_bin_spike, 2000000, NN_K, nn_idx, dd, NN_EPS / 5.0);

			for (int ni = 0; ni < NN_K; ++ni) {
				double spikex = obs_mat(nn_idx[ni], N_FEAT);
				if (abs(spikex - 1023) < 1){
					continue;
				}

				// TODO: optimze kernel computation and don't compute (x_s - x_b) each time
				long long logprob = kern_H_ax_(nn_idx[ni], coords_normalized(xb, 0), coords_normalized(yb, 1));
				logprob += feature_sum[nn_idx[ni]];

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

	// restore ax_point original coordinates
	p_bin_spike[N_FEAT] = x_s;
	p_bin_spike[N_FEAT + 1] = y_s;

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

	if (argc != 19){
		std::cout << "Exactly 18 parameters should be provided (starting with tetrode, ending with BASE_PATH)!";
		exit(1);
	}
	int *pars[] = {&tetr, &DIM, &NN_K, &NN_K_COORDS, &N_FEAT, &MULT_INT, &BIN_SIZE, &NBINS, &MIN_SPIKES, &SAMPLING_RATE, &BUFFER_SAMPLING_RATE, &BUFFER_LAST_PKG_ID, &SAMPLING_DELAY};

	for(int p=0; p < 13; ++p){
		*(pars[p]) = atoi(argv[p+1]);
	}
	NN_EPS = atof(argv[14]);
	SIGMA_X = atof(argv[15]);
	SIGMA_A = atof(argv[16]);
	SIGMA_XX = atof(argv[17]);
	BASE_PATH = argv[18];

	std::cout << "t " << tetr << ": SIGMA_X = " << SIGMA_X << ", SIGMA_A = " << SIGMA_A << "\n";

	std::cout << "t " << tetr << ": " << "start KDE estimation\n";

	// load trees, extract points, load mats
	std::ifstream kdstream(BASE_PATH + Utils::Converter::int2str(tetr) + ".kdtree");
	kdtree_ = new ANNkd_tree(kdstream);
	kdstream.close();
	ann_points_ = kdtree_->thePoints();

	obs_mat.load(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_obs.mat");
	pos_buf.load(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_pos_buf.mat");

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

	// Feature stds
	std::vector<float> stds;
	float avg_feat_std = .0f;
	std::cout << "t " << tetr << ": std of features: ";
	for (int f = 0; f < N_FEAT; ++f) {
		float stdf = arma::stddev(obs_mat.col(f));
		std::cout << stdf << " ";
		stds.push_back(stdf);
		avg_feat_std += stdf;
	}
	std::cout << "\n";
	avg_feat_std /= N_FEAT;

	float stdx = arma::stddev(obs_mat.col(N_FEAT));
	float stdy = arma::stddev(obs_mat.col(N_FEAT + 1));
	std::cout << "t " << tetr << ": " << "std of x  = " << stdx << "\n";
	std::cout << "t " << tetr << ": " << "std of y  = " << stdy << "\n";
	// normalize coords to have the average feature std
	for (int s = 0; s < total_spikes; ++s) {
		// ... loss of precision 1) from rounding to int; 2) by dividing int on float
		spike_coords_int(s, 0) = (int)(obs_mat(s, N_FEAT) * avg_feat_std / SIGMA_X * MULT_INT / stdx);  //= stdx / avg_feat_std;
		spike_coords_int(s, 1) = (int)(obs_mat(s, N_FEAT + 1) * avg_feat_std / SIGMA_X * MULT_INT / stdy);  //= stdy / avg_feat_std;

		ann_points_coords[s][0] = obs_mat(s, N_FEAT);
		ann_points_coords[s][1] = obs_mat(s, N_FEAT + 1);

		for (int f = 0; f < N_FEAT; ++f) {
			ann_points_int[s][f] = (int)round(obs_mat(s, f) / SIGMA_A * MULT_INT);
		}
	}

//	cache_spike_and_bin_neighbours();

	// BUILD TREE IN normalized (a, x) space
	// first normalize coordinates in obs_mat
	ax_points_ = annAllocPts(MIN_SPIKES, DIM + 2);
	for (int s = 0; s < total_spikes; ++s) {
		for (int f = 0; f < N_FEAT; ++f) {
			ax_points_[s][f] = obs_mat(s, f) / SIGMA_A * MULT_INT;
		}

		ax_points_[s][N_FEAT] = obs_mat(s, N_FEAT) / SIGMA_X * MULT_INT * avg_feat_std / stdx;
		ax_points_[s][N_FEAT + 1] = obs_mat(s, N_FEAT + 1) / SIGMA_X * MULT_INT * avg_feat_std / stdy;
	}
	kdtree_ax_ = new ANNkd_tree(ax_points_, total_spikes, DIM + 2);

	// spike location KDE
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			double xc = BIN_SIZE * (0.5 + xb);
			double yc = BIN_SIZE * (0.5 + yb);

			double kde_sum = 0;
			unsigned int npoints = 0;
			for (int n = 0; n < total_spikes; ++n) {
				if (abs(obs_mat(n, N_FEAT) - 1023) < 0.01){
					continue;
				}
				npoints ++;

				double sum = 0;
				double xdiff = (xc - obs_mat(n, N_FEAT)) / stdx / SIGMA_XX;
				sum += xdiff * xdiff;
				double ydiff = (yc - obs_mat(n, N_FEAT + 1)) / stdy / SIGMA_XX;
				sum += ydiff * ydiff;

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
	// overall tetrode average firing rate, spikes / s
	double mu = MIN_SPIKES * SAMPLING_RATE * BUFFER_SAMPLING_RATE / (BUFFER_LAST_PKG_ID - SAMPLING_DELAY);
	std::cout << "t " << tetr << ": Average firing rate: " << mu << "\n";
	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			double xc = BIN_SIZE * (0.5 + xb);
			double yc = BIN_SIZE * (0.5 + yb);

			double kde_sum = 0;
			unsigned int npoints = pos_buf.n_cols;
			for (int n = 0; n < pos_buf.n_cols; ++n) {
				double sum = 0;

				double xdiff = (xc - pos_buf(0, n)) / stdx / SIGMA_XX;
				double ydiff = (yc - pos_buf(1, n)) / stdy / SIGMA_XX;

				sum += xdiff * xdiff;
				sum += ydiff * ydiff;

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
		coords_normalized(xb, 0) = (int)((float)BIN_SIZE * (0.5 + xb) * avg_feat_std / SIGMA_X * MULT_INT / stdx);
		coords_normalized(xb, 1) = (int)((float)BIN_SIZE * (0.5 + xb) * avg_feat_std / SIGMA_X * MULT_INT / stdy);
	}
	std::cout << "t " << tetr << ": done coords_normalized\n";

	// prepare bin block neighbours cache - allocate
	NBLOCKS = NBINS / BLOCK_SIZE + ((NBINS % BLOCK_SIZE) ? 1 : 0);
	for (int xb = 0; xb < NBLOCKS; ++xb) {
		for (int yb = 0; yb < NBLOCKS; ++yb) {
			cache_block_neighbs_.push_back(new int[NN_K]);
		}
	}

	// compute p(a_i, x) for all spikes (as KDE of nearest neighbours)
	time_t start = clock();
	for (int p = 0; p < total_spikes; ++p) {
		// DEBUG
		if (!(p % 1000)){
			std::cout.precision(2);
			std::cout << "t " << tetr << ": " << p << " place fields built, last 1000 in " << (clock() - start)/ (float)CLOCKS_PER_SEC << " sec....\n";
			start = clock();

			// PROFILING
//			if (p >= 1000)
//				exit(0);
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
