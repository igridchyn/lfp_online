/*
 * CluReaderClusteringProcessor.h
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#ifndef CLUREADERCLUSTERINGPROCESSOR_H_
#define CLUREADERCLUSTERINGPROCESSOR_H_

#include "LFPProcessor.h"
#include <iostream>
#include <fstream>

class CluReaderClusteringProcessor : public virtual LFPProcessor {
	const std::string clu_path_;
	const std::string res_path_;

	std::ifstream clu_stream_;
	std::ifstream res_stream_;

	std::vector<unsigned int> max_clust_;
	std::vector<unsigned int> cluster_shifts_;

	unsigned int clust = 0, res = 0;

public:
	CluReaderClusteringProcessor(LFPBuffer *buffer);
	CluReaderClusteringProcessor(LFPBuffer *buffer, const std::string& clu_path_base,
			const std::string& res_path_base);

	virtual ~CluReaderClusteringProcessor();

	// LFPProcessor
	virtual void process();
	virtual inline std::string name() { return "Clu Reader Clustering"; }
};

#endif /* CLUREADERCLUSTERINGPROCESSOR_H_ */
