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

	unsigned int & pos_buf_ptr_;

	const bool SAVE;
	const bool LOAD;

	const bool SMOOTH;
	const bool USE_PARAMETRIC;
	const float SIGMA;
	const int SPREAD;

	std::vector<arma::fmat> trans_probs_;

	//DEBUG
	bool saved = false;

	// number of packages until sampling is over
	const unsigned int SAMPLING_END_;

	// new algorithm
	const unsigned int ESTIMATION_WINDOW;
	unsigned int from_pkg_id_;
	// positions from
	float xf_ = -1.0; float yf_ = -1.0;

	std::string tp_path_;

public:
	TransProbEstimationProcessor(LFPBuffer *buf);
	virtual ~TransProbEstimationProcessor();

	// LFPProcessor
	virtual void process();
	virtual inline std::string name() { return "TP Estimator"; }

private:
	bool interpolatedPositionAt(const unsigned int& pkg_id, const unsigned int& buf_pos, float& x, float& y);
};

#endif /* TRANSPROBESTIMATIONPROCESSOR_H_ */
