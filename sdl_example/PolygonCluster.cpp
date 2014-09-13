/*
 * PolygonCluster.cpp
 *
 *  Created on: Sep 12, 2014
 *      Author: igor
 */

#include "PolygonCluster.h"
#include <algorithm>

PolygonCluster::PolygonCluster() {

}

PolygonCluster::~PolygonCluster() {

}


// if point )x3, y3) is to the right from vector (x1, y2)->(x2, y2)
bool IsFromRight(float x1, float y1, float x2, float y2, float x3, float y3){

	// edge vector rotated 90 clock-wise
	float ex90 = y2 - y1;
	float ey90 = - (x2 - x1);

	float px = x3 - x1;
	float py = y3 - y1;

	// cos of angle between edge vector and vector from first vertex to test point
	float cossig = px * ex90 + py * ey90;

	return cossig < 0;
}

PolygonClusterProjection::PolygonClusterProjection(
		const std::vector<float> coords1, const std::vector<float> coords2, int dim1, int dim2)
: coords1_(coords1)
, coords2_(coords2)
, dim1_(dim1)
, dim2_(dim2){
	// inverse points if they go counter-clock-wise

	if (!IsFromRight(coords1[0], coords2[0], coords1[1], coords2[1], coords1[2], coords2[2])){
		// inverse points order
		std::reverse(coords1_.begin(), coords1_.end());
		std::reverse(coords2_.begin(), coords2_.end());
	}
}

PolygonCluster::PolygonCluster(const PolygonClusterProjection& proj) {
	projections_.push_back(proj);
}

bool PolygonCluster::Contains(float x, float y) {
	// TODO cash edge vectors
	// TODO all projections

	PolygonClusterProjection& pcp = projections_[0];
	int last = pcp.Size() - 1;

	for(int v=0; v < pcp.Size()-1; ++v){
//		float ex = pcp.coords1_[v+1] - pcp.coords1_[v];
//		float ey = pcp.coords2_[v+1] - pcp.coords2_[v];

		// edge rotated 90 clockwise
		if (!IsFromRight(pcp.coords1_[v], pcp.coords2_[v], pcp.coords1_[v+1], pcp.coords2_[v+1], x, y)){
			return false;
		}
	}

	return IsFromRight(pcp.coords1_[last], pcp.coords2_[last], pcp.coords1_[0], pcp.coords2_[0], x, y);
}

