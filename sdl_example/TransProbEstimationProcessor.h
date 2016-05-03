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

class TransProbEstimationProcessor: public virtual LFPProcessor {
	const unsigned int NBINSX;
	const unsigned int NBINSY;
	const unsigned int BIN_SIZE;
	const unsigned int NEIGHB_SIZE;

	const unsigned int STEP;
	const std::string BASE_PATH;

	// TODO !!! sync rewind
	unsigned int pos_buf_ptr_ = 0;

	const bool SAVE;
	const bool LOAD;

	const bool SMOOTH;
	const bool USE_PARAMETRIC;
	const float SIGMA;
	const int SPREAD;

	std::vector<arma::mat> trans_probs_;

	//DEBUG
	bool saved = false;

	// number of packages until sampling is over
	const unsigned int SAMPLING_END_;

public:
	TransProbEstimationProcessor(LFPBuffer *buf);
	TransProbEstimationProcessor(LFPBuffer *buf, const unsigned int nbinsx, const unsigned int nbinsy, const unsigned int bin_size,
			const unsigned int neighb_size, const unsigned int step, const std::string base_path, const bool save,
			const bool load, const bool smooth, const bool use_parametric, const float sigma, const int spread);
	virtual ~TransProbEstimationProcessor();

	// LFPProcessor
	virtual void process();
	virtual inline std::string name() { return "TP Estimator"; }
};

#endif /* TRANSPROBESTIMATIONPROCESSOR_H_ */
