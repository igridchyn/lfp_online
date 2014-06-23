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

	for (int t = 0; t < tetrn; ++t) {
		ann_points_[t] = annAllocPts(MIN_SPIKES, DIM);
		spike_place_fields_[t].reserve(MIN_SPIKES);

		ann_points_int_[t] = new int*[MIN_SPIKES];
		for (int d = 0; d < MIN_SPIKES; ++d) {
			ann_points_int_[t][d] = new int[DIM];
		}

		spike_coords_int_[t] = arma::Mat<int>(MIN_SPIKES, 2, arma::fill::zeros);
	}

	pf_built_.resize(tetrn);
}

KDClusteringProcessor::~KDClusteringProcessor() {
	// TODO Auto-generated destructor stub
}

void KDClusteringProcessor::build_pax_(const unsigned int tetr, const unsigned int spikei) {
	arma::mat pf(NBINS, NBINS, arma::fill::zeros);

	for (int xb = 0; xb < NBINS; ++xb) {
		for (int yb = 0; yb < NBINS; ++yb) {
			// order of >= 30
			long long kern_sum = 0;

			for (int ni = 0; ni < NN_K; ++ni) {
				kern_sum += kern_(spikei, knn_cache_[tetr][spikei][ni], tetr);
			}

			// scaled by MULT_INT ^ 2
			pf(xb, yb) = (double)kern_sum / (MIN_SPIKES ^ 2);
		}
	}

	spike_place_fields_[tetr][spikei] = pf;

	if (SAVE){
		std::string save_path = BASE_PATH + Utils::NUMBERS[tetr] + "_" + Utils::Converter::int2str((int)spikei) + ".mat";
		pf.save(save_path, arma::raw_ascii);
//		std::cout << save_path << "\n";
	}
}

int inline KDClusteringProcessor::kern_(const unsigned int spikei1, const unsigned int spikei2, const unsigned int tetr) {
	// TODO: implement efficiently (integer with high precision)
	// TODO: check for the overlow (MULT order X DIM X MAX(coord order / feature sorder) X squared = (10 + 1 + 2) * 2 = 26 < 32 bit )

	int sum = 0;
//	sum += (obs_spikes_[tetr][spikei1]->x - obs_spikes_[tetr][spikei2]->x) / X_STD;
//	sum += (obs_spikes_[tetr][spikei1]->y - obs_spikes_[tetr][spikei2]->y) / Y_STD;

	int *pcoord1 = ann_points_int_[tetr][spikei1], *pcoord2 = ann_points_int_[tetr][spikei2];
	for (int d = 0; d < DIM; ++d, ++pcoord1, ++pcoord2) {
		sum += (*pcoord1 - *pcoord2) ^ 2;
	}
	sum *=X_STD;

	// !!! ASSUMING THAT X_STD == Y_STD don't divide but rather multiply in sum by X_STD
	// X coordinate
	sum += (spike_coords_int_[tetr](spikei1, 0) - spike_coords_int_[tetr](spikei2, 0)) ^ 2;
	// Y coordinate
	sum += (spike_coords_int_[tetr](spikei1, 1) - spike_coords_int_[tetr](spikei2, 1)) ^ 2;

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

	return - (sum >> 1);
}

void KDClusteringProcessor::process(){
	while(buffer->spike_buf_pos_clust_ < buffer->spike_buf_pos){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_clust_];
		const unsigned int tetr = spike->tetrode_;

		if (!pf_built_[tetr]){
			if (total_spikes_[tetr] >= MIN_SPIKES){
				// build the kd-tree and call kNN for all points, cache indices (unsigned short ??) in the array of pointers to spikes
				std::cout << "build kd-tree for tetrode " << tetr << "... ";
				kdtrees_[tetr] = new ANNkd_tree(ann_points_[tetr], total_spikes_[tetr], DIM);
				std::cout << "done\n Cache " << NN_K << " nearest neighbours for each spike...";

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

				// compute p(a_i, x) for all spikes (from neighbours
				time_t start = clock();
				for (int p = 0; p < total_spikes_[tetr]; ++p) {
					// DEBUG
					if (!(p % 500)){
						std::cout.precision(2);
						std::cout << p << " place fields built, last 500 in " << (clock() - start)/ (float)CLOCKS_PER_SEC << " sec....\n";
						start = clock();

					// for profiling
	//					if (p > 2500)
	//						exit(0);
					}

					build_pax_(tetr, p);
				}

				pf_built_[tetr] = true;
			}
			else{
				obs_spikes_[tetr].push_back(spike);

				for (int pc=0; pc < 3; ++pc) {
					// TODO: tetrode channels
					for(int chan=0; chan < 4; ++chan){
						ann_points_[tetr][total_spikes_[tetr]][chan * 3 + pc] = spike->pc[chan][pc];

						// save integer with increased precision for integer KDE operations
						ann_points_int_[tetr][total_spikes_[tetr]][chan * 3 + pc] = (int)round(spike->pc[chan][pc] * MULT_INT);
						spike_coords_int_[tetr](total_spikes_[tetr], 0) = (int)round(spike->x * MULT_INT);
						spike_coords_int_[tetr](total_spikes_[tetr], 1) = (int)round(spike->y * MULT_INT);
					}
				}

				total_spikes_[tetr] ++;
			}
		}
		else{
			// predict from spike in window
		}

		buffer->spike_buf_pos_clust_ ++;
	}
}
