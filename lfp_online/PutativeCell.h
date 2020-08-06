/*
 * PutativeCell.h
 *
 *  Created on: Oct 27, 2016
 *      Author: igor
 */

#ifndef PUTATIVECELL_H_
#define PUTATIVECELL_H_

#include "Spike.h"
#include "PolygonCluster.h"
#include <vector>

enum WaveshapeType{
	WaveshapeTypeInterpolated,
	WaveshapeTypeOriginal
};

class WaveshapeCut{
public:
	int x1_;
	int y1_;
	int x2_;
	int y2_;

	unsigned int channel_;

	WaveshapeType waveshapeType_;

	WaveshapeCut(int x1, int y1, int x2, int y2, const unsigned int& channel, WaveshapeType waveshapeType)
		: x1_(x1)
		, y1_(y1)
		, x2_(x2)
		, y2_(y2)
		, channel_(channel)
		, waveshapeType_(waveshapeType)
	{}

	bool Contains(Spike *spike);
};

class PutativeCell{

public:
	PutativeCell();
	PutativeCell(PolygonCluster polygons);
	std::vector<WaveshapeCut> waveshape_cuts_;
	PolygonCluster polygons_;

	bool Contains(Spike *spike);
};

#endif /* PUTATIVECELL_H_ */
