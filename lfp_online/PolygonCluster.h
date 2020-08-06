/*
 * PolygonCluster.h
 *
 *  Created on: Sep 12, 2014
 *      Author: Igor Gridchyn
 */

#ifndef POLYGONCLUSTER_H_
#define POLYGONCLUSTER_H_


#include <vector>
#include <fstream>

#include "Spike.h"

enum PolygonClusterProjectionType{
	ExclusivePolygonClusterProjection,
	InclusivePolygonClusterProjection
};

class PolygonClusterProjection{
public:
	std::vector<float> coords1_, coords2_;
	int dim1_ = -1, dim2_ = -1;

	inline int Size() {return coords1_.size(); }
	PolygonClusterProjection(const std::vector<float> coords1, const std::vector<float> coords2, int dim1, int dim2);
	PolygonClusterProjection(std::ifstream& file);
	PolygonClusterProjection();
	~PolygonClusterProjection();

	bool Contains(float x, float y);

	void Serialize(std::ofstream& file);

	// inverse order CW <-> CCW
	void Invert();

	// whether is CW polygon (meaning that points from the right of the edges belong to the polygon)
	// heuristic based on number of angles < 180 degrees in clock-wise direction
	bool IsCW();
};

class PolygonCluster {
public:

	// 2 types of projections: inclusive and exclusive
	// 	to belong to a polygon cluster, a point must be:
	// 		* at least in ONE of it's inclusive projections
	// 		* outside of it's ALL exclusive projections

	std::vector<PolygonClusterProjection> projections_inclusive_;
	std::vector<PolygonClusterProjection> projections_exclusive_;

	inline int NProj() { return projections_inclusive_.size() + projections_exclusive_.size(); };
	bool ContainsConvex(float x, float y);
	bool Contains(float x, float y);
	bool Contains(Spike *s);
	void Serialize(std::ofstream& file);
	bool Empty();
	void Invalidate();

	PolygonCluster();
	PolygonCluster(const PolygonClusterProjection& proj);
	PolygonCluster(std::ifstream& file, unsigned int version);
	virtual ~PolygonCluster();
};

#endif /* POLYGONCLUSTER_H_ */
