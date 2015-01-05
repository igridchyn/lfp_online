/*
 * TransProbEstimationProcessor.h
 *
 *  Created on: Jun 28, 2014
 *      Author: igor
 */

#ifndef TRANSPROBESTIMATIONPROCESSOR_H_
#define TRANSPROBESTIMATIONPROCESSOR_H_

#include "LFPProcessor.h"

#include <armadillo>

class TransProbEstimationProcessor: public LFPProcessor {
	const unsigned int NBINSX;
	const unsigned int NBINSY;
	const unsigned int BIN_SIZE;
	const unsigned int NEIGHB_SIZE;

	const bool LOAD;
	const bool SAVE;

	const bool SMOOTH;
	const bool USE_PARAMETRIC;
	const float SIGMA;
	const int SPREAD;

	std::vector<arma::mat> trans_probs_;

	const unsigned int STEP;

	// TODO sync rewind
	unsigned int pos_buf_ptr_ = 0;

	const std::string BASE_PATH;

	//DEBUG
	bool saved = false;

public:
	TransProbEstimationProcessor(LFPBuffer *buf);
	TransProbEstimationProcessor(LFPBuffer *buf, const unsigned int nbinsx, const unsigned int nbinsy, const unsigned int bin_size,
			const unsigned int neighb_size, const unsigned int step, const std::string base_path, const bool save,
			const bool load, const bool smooth, const bool use_parametric, const float sigma, const int spread);
	virtual ~TransProbEstimationProcessor();

	// LFPProcessor
	virtual void process();
};

#endif /* TRANSPROBESTIMATIONPROCESSOR_H_ */
