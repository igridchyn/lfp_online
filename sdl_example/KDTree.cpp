/*
 * KDTree.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#include "KDTree.h"

KDTree::KDTree(const arma::mat& data, const unsigned int max_level) {
	// find the coordinate with the largest variance / (do PCA optionally)
	const unsigned int dimension = data.n_cols;
	const unsigned int npoints = data.n_rows;

	unsigned int max_var_dim = -1;
	double max_var = std::numeric_limits<double>::min();

	for (int i = 0; i < dimension; ++i) {
		double var = arma::var(data.col(i));

		if (var > max_var){
			max_var = var;
			max_var_dim = i;
		}
	}

	split_dim_ = max_var_dim;

	// sort along the selected coordinate / PC
	arma::uvec dim_sort_ind = arma::sort_index(data.col(max_var_dim));
	split_val_ = data(npoints / 2, max_var_dim);

	// choose median and split data
	// TODO: ? avoid recursion for better performance ?
	if (max_level > 0){
		left_ = new KDTree(data.rows(dim_sort_ind.rows(0, npoints / 2)), max_level - 1);
		right_ = new KDTree(data.rows(dim_sort_ind.rows(npoints / 2 + 1, npoints - 1)), max_level - 1);
	}
	else{
		left_ = right_ = NULL;
	}
}

KDTree::~KDTree() {
	// TODO Auto-generated destructor stub
}

