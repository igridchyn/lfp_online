//
//  PutativeCell.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 27/10/16.
//  Copyright (c) 2016 Igor Gridchyn. All rights reserved.
//

#include "PutativeCell.h"

bool WaveshapeCut::Contains(Spike *spike){
	if (waveshapeType_ == WaveshapeTypeOriginal){
		return spike->crossesWaveShapeFinal(channel_, x1_, y1_, x2_, y2_);
	}
	else{
		return spike->crossesWaveShapeReconstructed(channel_, x1_, y1_, x2_, y2_);
	}
}

bool PutativeCell::Contains(Spike *spike){
	bool polygonsContain = polygons_.Contains(spike);

	if (!polygonsContain){
		return false;
	}

	for (unsigned int c=0; c < waveshape_cuts_.size(); ++c){
		if (waveshape_cuts_[c].Contains(spike)){
			return false;
		}
	}

	return true;
}

PutativeCell::PutativeCell(PolygonCluster polyclust)
	:polygons_(polyclust)
{

}

PutativeCell::PutativeCell() {}
