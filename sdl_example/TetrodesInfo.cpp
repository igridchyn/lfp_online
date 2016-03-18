/*
 * TetrodesInfo.cpp
 *
 *  Created on: Sep 28, 2014
 *      Author: Igor Gridchyn
 */

#include "TetrodesInfo.h"

int TetrodesInfo::number_of_channels(Spike *spike) const{
    return tetrode_channels[spike->tetrode_].size();
}

TetrodesInfo* TetrodesInfo::GetInfoForTetrodesRange(const unsigned int& from, const unsigned int& to){
	assert(to >= from);
	TetrodesInfo * tetrinf = new TetrodesInfo();

	const unsigned int tetrn = to - from + 1;

	tetrinf->tetrode_channels.resize(tetrn);
	for (unsigned int t = 0; t < tetrn; ++t) {
		tetrinf->tetrode_channels[t].resize(4);
		for (unsigned int c = 0; c < 4; ++c) {
			tetrinf->tetrode_channels[t][c] = (from+t)*4 + c;
		}
	}

	return tetrinf;
}

TetrodesInfo* TetrodesInfo::GetMergedTetrodesInfo(const TetrodesInfo* ti1, const TetrodesInfo* ti2){
	TetrodesInfo * tetrinf = new TetrodesInfo();

	const unsigned int tetrn = ti1->tetrodes_number() + ti2->tetrodes_number();

	tetrinf->tetrode_channels.resize(tetrn);
	for (unsigned int t = 0; t < ti1->tetrodes_number(); ++t) {
		tetrinf->tetrode_channels[t].resize(ti1->tetrode_channels[t].size());
		for (unsigned int c = 0; c < ti1->tetrode_channels[t].size(); ++c){
			tetrinf->tetrode_channels[t][c] = ti1->tetrode_channels[t][c];
		}
	}

	const unsigned int shift = ti1->tetrodes_number();
	for (unsigned int t = 0; t < ti2->tetrodes_number(); ++t) {
			tetrinf->tetrode_channels[t + shift].resize(ti2->tetrode_channels[t].size());
			for (unsigned int c = 0; c < ti1->tetrode_channels[t].size(); ++c){
				tetrinf->tetrode_channels[t + shift][c] = ti1->tetrode_channels[t][c];
			}
	}

	return tetrinf;
}

TetrodesInfo::TetrodesInfo(std::string config_path, Utils::Logger* logger) {
	std::ifstream tconfig(config_path);

	unsigned int tetrn = 0;
	tconfig >> tetrn;

	logger->Log("Number of tetrodes in tetrode config: ", tetrn);

	if (tetrn <= 0){
		status_ = TI_STATUS_BAD_TETRODES_NUMBER;
		return;
	}

	tetrode_channels.resize(tetrn);

	int chnum;
	for (unsigned int t = 0; t < tetrn; ++t) {
		tconfig >> chnum;
		tetrode_channels[t].resize(chnum);
		for(int c=0; c < chnum; ++c){
			tconfig >> tetrode_channels[t][c];
		}
	}

	tconfig.close();

	// TODO: configurableize
	tetrode_label_map_.resize(32, INVALID_TETRODE);
	for (unsigned int t = 0; t < tetrn; ++t) {
		int abstetr = (int)floor(tetrode_channels[t][0] / 4);
		tetrode_label_map_[abstetr] = t;
	}

	status_ = TI_STATUS_LOADED;
}

TetrodesInfo::TetrodesInfo() {
}

TetrodesInfo::~TetrodesInfo() {
	delete[] tetrode_by_channel;
}

bool TetrodesInfo::ContainsChannel(const unsigned int& channel) {
	for (unsigned int t = 0; t < tetrodes_number(); ++t) {
		for (size_t c = 0; c < tetrode_channels[t].size(); ++c) {
			if (tetrode_channels[t][c] == channel)
				return true;
		}
	}

	return false;
}

bool TetrodesInfo::ContainsChannels(const std::vector<unsigned int>& channels) {
	for (size_t c = 0; c < channels.size(); ++c) {
		if (!ContainsChannel(c))
			return false;
	}
	return true;
}


unsigned int TetrodesInfo::Translate(const TetrodesInfo* const ti, const unsigned int& tetr) {
	int abstetr = (int)floor(ti->tetrode_channels[tetr][0] / 4);
	return tetrode_label_map_[abstetr];
}
