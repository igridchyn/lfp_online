/*
 * PolygonCluster.cpp
 *
 *  Created on: Sep 12, 2014
 *      Author: igor
 */

#include "PolygonCluster.h"
#include <algorithm>
#include "iostream"

PolygonCluster::PolygonCluster() {

}

void PolygonCluster::Serialize(std::ofstream& file) {
	file << projections_inclusive_.size() << "\n";
	for (size_t i=0; i < projections_inclusive_.size(); ++i){
		projections_inclusive_[i].Serialize(file);
	}
	file << projections_exclusive_.size() << "\n";
	for (size_t i=0; i < projections_exclusive_.size(); ++i){
		projections_exclusive_[i].Serialize(file);
	}
}

PolygonCluster::PolygonCluster(std::ifstream& file, unsigned int version) {
	if (version == 1){
		int sz = 0;
		file >> sz;
		for (int i=0; i < sz; ++i){
			projections_inclusive_.push_back(PolygonClusterProjection(file));
		}
		file >> sz;
		for (int i=0; i < sz; ++i){
			projections_exclusive_.push_back(PolygonClusterProjection(file));
		}
	}
}

bool PolygonCluster::Contains(float x, float y) {
	bool inclusive = false;

	for (size_t p = 0; p < projections_inclusive_.size(); ++p) {
		if (projections_inclusive_[p].Contains(x, y))
			inclusive = true;
			break;
	}

	if (!inclusive)
		return false;

	for (size_t p = 0; p < projections_exclusive_.size(); ++p) {
			if (projections_exclusive_[p].Contains(x, y))
				return false;
		}

	return true;
}

bool PolygonCluster::Contains(Spike* spike) {
	bool inclusive = false;

	for (size_t p=0; p < projections_inclusive_.size(); ++p){
		int dim1 = projections_inclusive_[p].dim1_;
		int dim2 = projections_inclusive_[p].dim2_;

		// should fall into either of the clusters
		if(projections_inclusive_[p].Contains(spike->getFeature(dim1),
				spike->getFeature(dim2))){
			inclusive = true;
			break;
		}
	}

	if (!inclusive)
		return false;

	for (size_t p=0; p < projections_exclusive_.size(); ++p){
		int dim1 = projections_exclusive_[p].dim1_;
		int dim2 = projections_exclusive_[p].dim2_;

		if(projections_exclusive_[p].Contains(spike->getFeature(dim1),
						spike->getFeature(dim2))){
			return false;
		}
	}

	return true;
}

bool PolygonCluster::Empty() {
	return projections_inclusive_.size() == 0;
}

PolygonCluster::~PolygonCluster() {

}


// if point (x3, y3) is to the right from vector (x1, y2)->(x2, y2)
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

	// duplicate first point for processing simplicity
	if (coords1_.size() > 0) {
		coords1_.push_back(coords1_[0]);
		coords2_.push_back(coords2_[0]);

		if (!IsCW()){
			// inverse points order
			std::reverse(coords1_.begin(), coords1_.end());
			std::reverse(coords2_.begin(), coords2_.end());
		}
	}
}

PolygonCluster::PolygonCluster(const PolygonClusterProjection& proj) {
	projections_inclusive_.push_back(proj);
}

bool PolygonCluster::ContainsConvex(float x, float y) {
	// TODO cash edge vectors

	for (size_t p = 0; p < projections_inclusive_.size(); ++p){
		PolygonClusterProjection& pcp = projections_inclusive_[p];

		for(int v=1; v < pcp.Size(); ++v){
			if (!IsFromRight(pcp.coords1_[v-1], pcp.coords2_[v-1], pcp.coords1_[v], pcp.coords2_[v], x, y)){
				return false;
			}
		}
	}

	return true;
}

PolygonClusterProjection::PolygonClusterProjection(std::ifstream& file) {
	int size = 0;
	file >> dim1_ >> dim2_ >> size;
	coords1_.resize(size);
	coords2_.resize(size);
	for(int i=0; i < size; ++i){
		file >> coords1_[i] >> coords2_[i];
	}
}

bool PolygonClusterProjection::Contains(float x, float y) {
	// find closest edge to the left from point
	const unsigned int UNK = std::numeric_limits<unsigned int>::max();
	unsigned int pivot = UNK;
	float mindist = std::numeric_limits<float>::max();
	for(size_t i=1; i<coords1_.size();++i){
		if ( (coords2_[i] - y) * (coords2_[i-1] - y) >= 0 ){
			continue;
		}

		float pintx = coords1_[i - 1] + (coords1_[i] - coords1_[i-1]) / (coords2_[i] - coords2_[i-1]) * (y - coords2_[i-1]);
		if ((pintx < x) && (x - pintx <  mindist)){
			pivot = i;
			mindist = x - pintx;
		}
	}

	if (pivot == UNK)
		return false;

	// pivot intersection x
	// denom <. 0 because of true of if above (a point between them exists)

	// define if it is to the left or to the right (just by which one is higher)
	bool right = IsFromRight(coords1_[pivot-1], coords2_[pivot-1], coords1_[pivot], coords2_[pivot], x, y);

	return right;
}

//bool PolygonClusterProjection::Contains(float x, float y) {
//	// find the first edge s.t. point's y falls between edge's ends
//	const unsigned int UNK = std::numeric_limits<unsigned int>::max();
//	unsigned int pivot = UNK;
//	for(size_t i=1; i<coords1_.size();++i){
//		if ( (coords2_[i] - y) * (coords2_[i-1] - y) < 0 ){
//			pivot = i;
//		}
//	}
//
//	if (pivot == UNK)
//		return false;
//
//	// pivot intersection x
//	// denom <. 0 because of true of if above (a point between them exists)
//	float pintx = coords1_[pivot-1] + (coords1_[pivot] - coords1_[pivot-1]) / (coords2_[pivot] - coords2_[pivot-1]) * (y - coords2_[pivot-1]);
//
//	// define if it is to the left or to the right (just by which one is higher)
//	bool left = !IsFromRight(coords1_[pivot-1], coords2_[pivot-1], coords1_[pivot], coords2_[pivot], x, y);
//
//	// count how many times the line to the left/right from the point crosses other edges
//	int ncross = 0;
//	for(size_t i=1; i<coords1_.size();++i){
//		// edge's y on one side of point's y
//		if ( (coords2_[i] - y) * (coords2_[i-1] - y) >= 0 ){
//			continue;
//		}
//
//		// to avoid problems with precision
//		if (i == pivot){
//			continue;
//		}
//
//		// find x of intersection
//		float intx = coords1_[i-1] + (coords1_[i] - coords1_[i-1]) / (coords2_[i] - coords2_[i-1]) * (y - coords2_[i-1]);
//		if ( (x - intx) * (pintx - intx) < 0 ){
//			ncross ++;
//		}
//	}
//
//	return (ncross % 2 == 0) ^ left;
//}

void PolygonClusterProjection::Serialize(std::ofstream& file) {
	file << dim1_ << " " << dim2_ << " " << coords1_.size() << "\n";
	for(int i=0; i < Size(); ++i){
		file << coords1_[i] << " " << coords2_[i] << " ";
	}
	file << "\n";
}

void PolygonClusterProjection::Invert() {
	std::reverse(coords1_.begin(), coords1_.end());
	std::reverse(coords2_.begin(), coords2_.end());
}

PolygonClusterProjection::~PolygonClusterProjection() {
	// DEBUG
	coords1_.clear();
	coords2_.clear();
}

bool PolygonClusterProjection::IsCW() {
	unsigned int nright = 0;

	for (size_t c=0; c < coords1_.size()-2; ++c){
		if (IsFromRight(coords1_[c], coords2_[c], coords1_[c + 1], coords2_[c + 1], coords1_[c + 2], coords2_[c + 2])){
			nright ++;
		}
	}

	return nright >= coords1_.size() / 2;
}

PolygonClusterProjection::PolygonClusterProjection()
{}

void PolygonCluster::Invalidate() {
	projections_exclusive_.clear();
	projections_inclusive_.clear();
}
