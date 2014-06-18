/*
 * KDTree.h
 *
 *  Created on: Jun 18, 2014
 *      Author: igor
 */

#ifndef KDTREE_H_
#define KDTREE_H_

#include <armadillo>

class KDTree {
	// dimension of split in this node
	unsigned int split_dim_;

	// median
	double split_val_;

	KDTree *left_ = NULL;
	KDTree *right_ = NULL;

public:
	KDTree(const arma::mat& data, const unsigned int max_level);
	virtual ~KDTree();
};

#endif /* KDTREE_H_ */
