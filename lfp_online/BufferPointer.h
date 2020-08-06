/*
 * BufferPointer.h
 *
 *  Created on: Sep 16, 2014
 *      Author: igor
 */

#ifndef BUFFERPOINTER_H_
#define BUFFERPOINTER_H_

#include "LFPBuffer.h"

class SpikeBufferPointer {
	LFPBuffer *buf;

	unsigned int pos_;
	unsigned int cycles_ = 0;
	SpikeBufferPointer  *limit_pointer_;

public:
	SpikeBufferPointer ();

	void Advance();
	Spike* GetCurrent();

	virtual ~SpikeBufferPointer ();
};

#endif /* BUFFERPOINTER_H_ */
