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

#include "Utils.h"

#include <map>

int DIM;
int NN_K;
int NN_K_COORDS;
int N_FEAT;
int MULT_INT;
double BIN_SIZE;
int NBINSX;
int NBINSY;

const unsigned int BLOCK_SIZE = 3;
int NBLOCKSX;
int NBLOCKSY;

int MIN_SPIKES;
int SAMPLING_RATE;
int BUFFER_SAMPLING_RATE;
int BUFFER_LAST_PKG_ID;
int SAMPLING_DELAY;

double NN_EPS;
double SIGMA_X;
double SIGMA_A;
double SIGMA_XX;

double SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD;
double SPIKE_GRAPH_COVER_NNEIGHB;

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
arma::Mat<int> spike_coords_int, coords_normalized;
arma::fmat pos_buf;
// this one is loaded
arma::fmat obs_mat;
// these are created
arma::mat px(NBINSX, NBINSY, arma::fill::zeros), lx(NBINSX, NBINSY, arma::fill::zeros),
		pix(NBINSX, NBINSY, arma::fill::zeros), pix_log(NBINSX, NBINSY, arma::fill::zeros);

std::vector<arma::fmat> lax;

int **ann_points_int;

ANNpointArray ann_points_;
ANNpointArray ax_points_;
ANNpointArray ann_points_coords;

int total_spikes = 0;

std::stringstream log_string_;
std::ofstream log_;

unsigned int NUSED = 0;


void Log(std::string s){
	log_ << "t" << tetr << ": " << s;
	std::cout << "t" << tetr << ": " << s;
}

void Log(){
	log_ << "t" << tetr << ": " << log_string_.str();
	std::cout << "t" << tetr << ": " << log_string_.str();
	log_string_.str(std::string());
	log_.flush();
}

class VertexNode{
public:
	std::vector<unsigned int> neighbour_ids_;
	unsigned int id_;
	VertexNode *next_, *previous_;

	const unsigned int Size() const {return neighbour_ids_.size();}
	const bool operator<(const VertexNode& sample) const;

	VertexNode & operator=(VertexNode && ref);

	VertexNode(const unsigned int& id, const std::vector<unsigned int> neighbour_ids)
	: neighbour_ids_(neighbour_ids)
	 , id_(id){}

	VertexNode(const VertexNode& ref);
};

class VertexCoverSolver{
	// i-th entry is list of neighbour IDs of the i-th spike
	// can by asymmetric if approximate sear\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ch algorithm is used !!!

public:
	std::vector<unsigned int> Reduce(ANNkd_tree & full_tree, const double & threshold, const unsigned int& NNEIGB);
};


std::vector<unsigned int> VertexCoverSolver::Reduce(ANNkd_tree& full_tree,
		const double& threshold, const unsigned int& NNEIGB) {

	VertexNode *head;
	std::map<unsigned int, VertexNode *> node_by_id;
	// nodes having an index node in their neighbours lists [because due to approximate neighbour search it can be asymmetric]
	std::vector< std::vector< unsigned int > > neighbours_of;
	// create map (for removing neighbours), vector (for sorting) and list (for supporting sorting order)
	std::vector<VertexNode> nodes;

	// TODO !!! CHECK
	const double NN_EPS = 0.1; // 0.01
	ANNidx *nn_idx = new ANNidx[NNEIGB];
	ANNdist *dd = new ANNdist[NNEIGB];
	ANNpointArray tree_points = full_tree.thePoints();

	const unsigned int NPOITNS = full_tree.nPoints();

	neighbours_of.resize(NPOITNS);

	// create Vertex Nodex for each spike
	for (unsigned int n = 0; n < NPOITNS; ++n) {
		// find neighbours
		full_tree.annkSearch(tree_points[n], NNEIGB, nn_idx, dd, NN_EPS);

		// create VertexNode structure
		std::vector<unsigned int> neigb_ids;
		for (unsigned int ne = 1; ne < NNEIGB; ++ne) {
			if (dd[ne] > threshold){
				break;
			}

			neigb_ids.push_back((unsigned int)nn_idx[ne]);
			neighbours_of[nn_idx[ne]].push_back(n);
		}

		nodes.push_back(VertexNode(n, neigb_ids));
	}

	Log(": cached neighbours (vertex cover)\n");

	// optinally: make neiughbours of and lists in VertexNodes equal

	// sort them by number of neighbours less than thold
	 std::sort(nodes.begin(), nodes.end());

	 for (unsigned int n = 0; n < NPOITNS; ++n) {
		 node_by_id[nodes[n].id_] = &(nodes[n]);
	 }

	 log_string_ << ": node with most neighbours: " << nodes[0].neighbour_ids_.size() << "\n\t";
	 log_string_ << ": node with higher quartile neighbours: " << nodes[nodes.size()/4].neighbour_ids_.size() << "\n\t";
	 log_string_ << ": node with mean neighbours: " << nodes[nodes.size()/2].neighbour_ids_.size() << "\n\t";
	 log_string_ << ": node with lower quartile neighbours: " << nodes[3*nodes.size()/4].neighbour_ids_.size() << "\n\t";
	 log_string_ << ": node with least neighbours: " << nodes[nodes.size() - 1].neighbour_ids_.size() << "\n";
	 Log();

	 // create sorted double-linked list
	 head = &nodes[0];
	 head->previous_ = nullptr;
	 VertexNode *last = head;
	 for (unsigned int n = 1; n < NPOITNS; ++n) {
		 last->next_ = &nodes[n];
		 nodes[n].previous_ = last;
		 last = &nodes[n];
	}
	last->next_ = nullptr;

	// while list is not empty, choose node with largest number of neighbours and remove it and it's neighborus from list and map
	std::vector<unsigned int> used_ids_;
	while (head != nullptr ){ // && head->neighbour_ids_.size() > 0){
		// add one with most neighbours to the list of active nodes (to build PFs from)
		used_ids_.push_back(head->id_);

		// remove neighbours from map /	list and reduce neighbour counts of neighbours' neighbours
		for (size_t i=0; i < head->neighbour_ids_.size(); ++i){
			// remove and move down in the sorted list
			const unsigned int neighb_id =head->neighbour_ids_[i];
			VertexNode *const neighbour = node_by_id[neighb_id];

			if (neighbour == nullptr)
				continue;

			// remove from neighbour's list at each neighbour neighbour and move it down
			for (size_t j = 0; j < neighbours_of[neighbour->id_].size(); ++j) {
				unsigned int jthneighbid = neighbours_of[neighbour->id_][j];

				if (jthneighbid == head->id_)
					continue;

				VertexNode *nn = node_by_id[jthneighbid];

				if (nn == nullptr)
					continue;

				// TODO don't remove, just have the number of active neighbour nodes
				if (nn->neighbour_ids_.size() > 0){
					std::remove(nn->neighbour_ids_.begin(), nn->neighbour_ids_.end(), neighbour->id_);
				}else{
					Log("no neighbours!\n");
				}

				//also remove NN from reverse neighbours list of 2nd order neighbours
				std::remove(neighbours_of[jthneighbid].begin(), neighbours_of[jthneighbid].end(), neighb_id);

				// move down in the list to support the sorted order
				while (nn->next_ !=0 && nn->Size() < nn->next_->Size()){
					VertexNode *tmp = nn->next_;
					nn->next_ = tmp->next_;
					nn->previous_->next_ = tmp;
					tmp->next_ = neighbour;
					tmp->previous_ = nn->previous_;
					nn->previous_ = tmp;
					nn->next_->previous_ = nn;
				}
			}

			// remove neighbour from the list
			if (node_by_id[neighbour->id_]->previous_ != nullptr)
				node_by_id[neighbour->id_]->previous_->next_ = node_by_id[neighbour->id_]->next_;

			if (node_by_id[neighbour->id_]->next_ != nullptr)
				node_by_id[neighbour->id_]->next_->previous_ = node_by_id[neighbour->id_]->previous_;

			node_by_id[neighbour->id_] = nullptr;

			neighbours_of[neighbour->id_].clear();
		}

		// move to next node with most neighbours
		head = head->next_;
	}

	delete[] nn_idx;
	delete[] dd;

	return used_ids_;
}

const bool VertexNode::operator <(const VertexNode& sample) const{
	return Size() > sample.Size();
}

VertexNode& VertexNode::operator =(VertexNode&& ref) {
	id_ = ref.id_;
	// to be used only while sorting !
	// TODO throw if not null ?
	next_ = nullptr;
	previous_ = nullptr;
	neighbour_ids_ = ref.neighbour_ids_;

	return *this;
}

VertexNode::VertexNode(const VertexNode& ref) {
	id_ = ref.id_;
	next_ = nullptr;
	previous_ = nullptr;
	neighbour_ids_ = ref.neighbour_ids_;
}

void cache_spike_and_bin_neighbours(){

	// CACHE spikes with closest WS
	// number of neihgbours to be cached - both for spikes and bin centers
	const unsigned int NN_CACHE = 1000;
	// cache spike neighbours
	ANNpointArray ann_points_ws = kdtree_->thePoints();
	for (int s = 0; s < total_spikes; ++s) {
		ANNidx *nn_idx = new ANNidx[NN_CACHE];
		ANNdist *dd = new ANNdist[NN_CACHE];
		kdtree_->annkSearch(ann_points_ws[s], NN_CACHE, nn_idx, dd, NN_EPS);
		cache_sim_spikes_.push_back(nn_idx);
		cache_sim_spikes_dists_.push_back(dd);
	}

	// build kd-tree in the (x, y) space
	kd_tree_coords_ = new ANNkd_tree(ann_points_coords, total_spikes, 2);
	// cache neighbours for each bin center
	for (int xb = 0; xb < NBINSX; ++xb) {
			for (int yb = 0; yb < NBINSY; ++yb) {
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
	Log("Done (x_b, y_b) neighbours and distances caching\n");
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

void build_pax_(const unsigned int& tetr, const unsigned int& spikei, const arma::mat& occupancy, const double& avg_firing_rate) {
	arma::fmat pf(NBINSX, NBINSY, arma::fill::zeros);

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

	for (int xb = 0; xb < NBINSX; ++xb) {
		for (int yb = 0; yb < NBINSY; ++yb) {
			// order of >= 30
			double kern_sum = 0;

			unsigned int nspikes = 0;
			// compute KDE (proportional to log-probability) over cached nearest neighbours
			// TODO ? exclude self from neighbours list ?

			// do kd search if first bin in the block, otherwise - use cached neighbours
			// find closest points for 'spike with a given sha'
			int nblock = (yb / BLOCK_SIZE) * NBLOCKSX + xb / BLOCK_SIZE;
			if (!(xb % BLOCK_SIZE) && !(yb % BLOCK_SIZE)){
				// form a point (a_i, x_b, y_b) and find its nearest neighbours in (a, x) space for KDE
				// have to choose central bin of the block
				p_bin_spike[N_FEAT] = coords_normalized(xb < NBINSX - 1 ? xb + 1 : xb, 0);
				p_bin_spike[N_FEAT + 1] = coords_normalized(yb < NBINSY - 1 ? yb + 1 : yb, 1);

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

				// don't use too fat apart neighbours for the estimation
//				if (dd[ni] > dd[0] * 2){
//					break;
//				}

				// TODO: optimze kernel computation and don't compute (x_s - x_b) each time
				long long logprob = kern_H_ax_(nn_idx[ni], coords_normalized(xb, 0), coords_normalized(yb, 1));
				logprob += feature_sum[nn_idx[ni]];

				nspikes++;
				kern_sum += exp((double)logprob / (MULT_INT * MULT_INT));
			}

			// scaled by MULT_INT ^ 2
//			pf(xb, yb) = (double)kern_sum / (MULT_INT * MULT_INT) / nspikes;
			if (occupancy(xb, yb)/occ_sum >= MIN_OCC){
				pf(xb, yb) = log(kern_sum / nspikes / occupancy(xb, yb) * avg_firing_rate);
			}
		}
	}

	// restore ax_point original coordinates
	p_bin_spike[N_FEAT] = x_s;
	p_bin_spike[N_FEAT + 1] = y_s;

	float occ_min = pf.min();
	// set min at low occupancy
	for (int xb = 0; xb < NBINSX; ++xb) {
		for (int yb = 0; yb < NBINSY; ++yb) {
			if (occupancy(xb, yb)/occ_sum < MIN_OCC){
				pf(xb, yb) = occ_min;
			}
		}
	}

	// normalize to sum up to 1 - did
//	arma::mat pfexp = arma::exp(pf);
//	double pfsum = arma::sum(arma::sum(pfexp));
//	pfexp /= pfsum;
//	pf = arma::log(pfexp);

	lax[spikei] = pf;
}

int main(int argc, char **argv){
	// read the following variables and perform KDE estimation and tree building

//	std::cout << "build kd-tree for tetrode " << tetr;
//	kdtree_ = new ANNkd_tree(ann_points_, total_spikes_, DIM);
//	std::cout << "done\n Cache " << NN_K << " nearest neighbours for each spike...\n";

	if (argc != 22){
		Log("Exactly 21 parameters should be provided (starting with tetrode, ending with BASE_PATH)!");
		exit(1);
	}
	int *pars[] = {&tetr, &DIM, &NN_K, &NN_K_COORDS, &N_FEAT, &MULT_INT, &NBINSX, &NBINSY, &MIN_SPIKES, &SAMPLING_RATE, &BUFFER_SAMPLING_RATE, &BUFFER_LAST_PKG_ID, &SAMPLING_DELAY};

	for(int p=0; p < 13; ++p){
		*(pars[p]) = atoi(argv[p+1]);
	}
	BIN_SIZE = atof(argv[14]);
	NN_EPS = atof(argv[15]);
	SIGMA_X = atof(argv[16]);
	SIGMA_A = atof(argv[17]);
	SIGMA_XX = atof(argv[18]);
	SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD = atof(argv[19]);
	SPIKE_GRAPH_COVER_NNEIGHB = atof(argv[20]);
	BASE_PATH = argv[21];

	log_string_ << "SIGMA_X = " << SIGMA_X << ", SIGMA_A = " << SIGMA_A << ", VC_THOLD = " << SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD << ", VC_NNEIGHB = " << SPIKE_GRAPH_COVER_NNEIGHB << "\n";
	log_string_ << "\tstart KDE estimation\n";
	Log();

	// load trees, extract points, load mats
	std::ifstream kdstream(BASE_PATH + Utils::Converter::int2str(tetr) + ".kdtree");
	kdtree_ = new ANNkd_tree(kdstream);
	kdstream.close();
	ann_points_ = kdtree_->thePoints();

	log_.open(BASE_PATH + "KDE_tetr_" + Utils::NUMBERS[tetr] + ".log");

	obs_mat.load(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_obs.mat");
	pos_buf.load(BASE_PATH + "tmp_" + Utils::NUMBERS[tetr] + "_pos_buf.mat");

	// DEBUG
	log_string_ << "obs " << obs_mat.n_rows << " X " << obs_mat.n_cols << "\n";
	log_string_ << "pos " << pos_buf.n_rows << " X " << pos_buf.n_cols << "\n";
	Log();

	total_spikes = obs_mat.n_rows;

	// allocate estimation matrices
	px = arma::mat(NBINSX, NBINSY, arma::fill::zeros);
	lx = arma::mat(NBINSX, NBINSY, arma::fill::zeros);
	pix = arma::mat(NBINSX, NBINSY, arma::fill::zeros);
	pix_log = arma::mat(NBINSX, NBINSY, arma::fill::zeros);
	lax.resize(MIN_SPIKES,  arma::fmat(NBINSX, NBINSY, arma::fill::zeros));

	ann_points_coords = annAllocPts(MIN_SPIKES, 2);
	spike_coords_int = arma::Mat<int>(total_spikes, 2);
	coords_normalized = arma::Mat<int>(std::max(NBINSX, NBINSY), 2);
	ann_points_int = new int*[MIN_SPIKES];
	for (int d = 0; d < MIN_SPIKES; ++d) {
		ann_points_int[d] = new int[DIM];
	}

	// Feature stds
	std::vector<float> stds;
	float avg_feat_std = .0f;
	log_string_ << "std of features: ";
	for (int f = 0; f < N_FEAT; ++f) {
		float stdf = arma::stddev(obs_mat.col(f));
		log_string_ << stdf << " ";
		stds.push_back(stdf);
		avg_feat_std += stdf;
	}
	log_string_ << "\n";
	avg_feat_std /= N_FEAT;
	Log();

	float stdx = arma::stddev(obs_mat.col(N_FEAT));
	float stdy = arma::stddev(obs_mat.col(N_FEAT + 1));
	log_string_ << "std of x  = " << stdx << "\n";
	log_string_ << "std of y  = " << stdy << "\n";
	Log();
	// normalize coords to have the average feature std
	for (int s = 0; s < total_spikes; ++s) {
		// ... loss of precision 1) from rounding to int; 2) by dividing int on float
		spike_coords_int(s, 0) = (int)(obs_mat(s, N_FEAT) * avg_feat_std / SIGMA_X * MULT_INT / stdx);  //= stdx / avg_feat_std;
		spike_coords_int(s, 1) = (int)(obs_mat(s, N_FEAT + 1) * avg_feat_std / SIGMA_X * MULT_INT / stdy);  //= stdy / avg_feat_std;

		ann_points_coords[s][0] = obs_mat(s, N_FEAT);
		ann_points_coords[s][1] = obs_mat(s, N_FEAT + 1);

		for (int f = 0; f < N_FEAT; ++f) {
			ann_points_int[s][f] = (int)round(obs_mat(s, f) / SIGMA_A * MULT_INT); // * avg_feat_std / stds[f];
		}
	}

//	cache_spike_and_bin_neighbours();

	// BUILD TREE IN normalized (a, x) space
	// first normalize coordinates in obs_mat
	ax_points_ = annAllocPts(MIN_SPIKES, DIM + 2);
	for (int s = 0; s < total_spikes; ++s) {
		for (int f = 0; f < N_FEAT; ++f) {
			ax_points_[s][f] = obs_mat(s, f) / SIGMA_A * MULT_INT; // * avg_feat_std / stds[f];
		}

		ax_points_[s][N_FEAT] = obs_mat(s, N_FEAT) / SIGMA_X * MULT_INT * avg_feat_std / stdx;
		ax_points_[s][N_FEAT + 1] = obs_mat(s, N_FEAT + 1) / SIGMA_X * MULT_INT * avg_feat_std / stdy;
	}
	kdtree_ax_ = new ANNkd_tree(ax_points_, total_spikes, DIM + 2);

	// spike location KDE
	for (int xb = 0; xb < NBINSX; ++xb) {
		for (int yb = 0; yb < NBINSY; ++yb) {
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
	double mu = MIN_SPIKES * SAMPLING_RATE * BUFFER_SAMPLING_RATE / double(BUFFER_LAST_PKG_ID - SAMPLING_DELAY);
	log_string_ << ": Average firing rate: " << mu << "\n";
	Log();
	for (int xb = 0; xb < NBINSX; ++xb) {
		for (int yb = 0; yb < NBINSY; ++yb) {
			double xc = BIN_SIZE * (0.5 + xb);
			double yc = BIN_SIZE * (0.5 + yb);

			double kde_sum = 0;
			unsigned int npoints = pos_buf.n_cols;
			for (unsigned int n = 0; n < pos_buf.n_cols; ++n) {
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
	Log(": done pix\n");

	// compute generalized rate function lambda(x)
	// TODO !!! uniform + parametrize
	const double MIN_OCC = 0.0001;
	double pisum = arma::sum(arma::sum(pix));
	for (int xb = 0; xb < NBINSX; ++xb) {
		for (int yb = 0; yb < NBINSY; ++yb) {
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
	for (int b = 0; b < std::max(NBINSX, NBINSY); ++b) {
		coords_normalized(b, 0) = (int)(BIN_SIZE * (0.5 + b) * avg_feat_std / SIGMA_X * MULT_INT / stdx);
		coords_normalized(b, 1) = (int)(BIN_SIZE * (0.5 + b) * avg_feat_std / SIGMA_X * MULT_INT / stdy);
	}
	Log(": done coords_normalized\n");

	// prepare bin block neighbours cache - allocate
	NBLOCKSX = NBINSX / BLOCK_SIZE + ((NBINSX % BLOCK_SIZE) ? 1 : 0);
	NBLOCKSY = NBINSY / BLOCK_SIZE + ((NBINSY % BLOCK_SIZE) ? 1 : 0);
	for (int xb = 0; xb < NBLOCKSX; ++xb) {
		for (int yb = 0; yb < NBLOCKSY; ++yb) {
			cache_block_neighbs_.push_back(new int[NN_K]);
		}
	}

	// reduce the tree by solving vertex cover
	VertexCoverSolver ver_solv;
	log_string_ << "run vertex cover solver with distance threshold = " <<  SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD << "\n\t and # of neighbours limited to " << SPIKE_GRAPH_COVER_NNEIGHB << "\n";
	Log();
	std::vector<unsigned int> used_ids_ = ver_solv.Reduce(*kdtree_, SPIKE_GRAPH_COVER_DISTANCE_THRESHOLD, SPIKE_GRAPH_COVER_NNEIGHB);// form new tree with only used points
	log_string_ << "done vertex cover, # of points in reduced tree: " << used_ids_.size() << "\n";
	Log();
	NUSED = used_ids_.size();

	// construct and save the reduced tree
	ANNpointArray tree_points = kdtree_->thePoints();
	ANNpointArray reduced_array = annAllocPts(used_ids_.size(), DIM);
	for (size_t i=0; i < used_ids_.size(); ++i){
		for (int f=0; f < DIM; ++f){
			reduced_array[i][f] = tree_points[used_ids_[i]][f];
		}
	}

	// save to be used during decoding
	ANNkd_tree *reduced_tree = new ANNkd_tree(reduced_array, used_ids_.size(), DIM);
	std::ofstream kdstream_reduced(BASE_PATH + Utils::Converter::int2str(tetr) + ".kdtree.reduced");
	reduced_tree->Dump(ANNtrue, kdstream_reduced);
	kdstream_reduced.close();
	delete reduced_tree;

	// compute p(a_i, x) for all spikes (as KDE of nearest neighbours)
	time_t start = clock();
	//for (int p = 0; p < total_spikes; ++p) {
	for (unsigned int u=0; u < NUSED; ++u){
		int p = used_ids_[u];

		// DEBUG
		if (!(u % 1000)){
			std::cout.precision(2);
			log_string_ << u << " out of  " << NUSED << " place fields built, last 1000 in " << (clock() - start)/ (float)CLOCKS_PER_SEC << " sec....\n";
			Log();
			start = clock();
		}

		build_pax_(tetr, p, pix, mu);
	}

	// if save - concatenate all matrices laxs_[tetr] and save (along with individual, for fast visualization)
	if (SAVE){
		arma::fmat laxs_tetr_(NBINSX, NBINSY * NUSED);
		for (unsigned int s = 0; s < NUSED; ++s) {
			laxs_tetr_.cols(s*NBINSY, (s + 1) * NBINSY - 1) = lax[used_ids_[s]];
		}
		laxs_tetr_.save(BASE_PATH + Utils::NUMBERS[tetr] + "_tetr.mat");
	}

	log_string_ << "FINISHED\n";
	Log();

	return 0;
}
