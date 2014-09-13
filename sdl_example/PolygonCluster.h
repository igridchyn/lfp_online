/*
 * PolygonCluster.h
 *
 *  Created on: Sep 12, 2014
 *      Author: Igor Gridchyn
 */

#ifndef POLYGONCLUSTER_H_
#define POLYGONCLUSTER_H_

#include <vector>

class PolygonClusterProjection{
public:
	int dim1_, dim2_;
	std::vector<float> coords1_, coords2_;;

	inline int Size() {return coords1_.size(); }
	PolygonClusterProjection(const std::vector<float> coords1, const std::vector<float> coords2, int dim1, int dim2);
};

class PolygonCluster {
public:
	std::vector<PolygonClusterProjection> projections_;

	inline int NProj() { return projections_.size(); };
	bool Contains(float x, float y);

	PolygonCluster();
	PolygonCluster(const PolygonClusterProjection& proj);
	virtual ~PolygonCluster();
};

#endif /* POLYGONCLUSTER_H_ */
