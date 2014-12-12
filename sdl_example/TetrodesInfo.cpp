/*
 * TetrodesInfo.cpp
 *
 *  Created on: Sep 28, 2014
 *      Author: Igor Gridchyn
 */

#include "TetrodesInfo.h"

int TetrodesInfo::number_of_channels(Spike *spike) const{
    return channels_numbers[spike->tetrode_];
}

TetrodesInfo* TetrodesInfo::GetInfoForTetrodesRange(const unsigned int& from, const unsigned int& to){
	assert(to >= from);
	TetrodesInfo * tetrinf = new TetrodesInfo();

	const unsigned int tetrn = to - from + 1;

	tetrinf->tetrodes_number = tetrn;
	tetrinf->channels_numbers = new int[tetrn];
	tetrinf->tetrode_channels = new int*[tetrn];

	for (int t = 0; t < tetrn; ++t) {
		tetrinf->channels_numbers[t] = 4;
		tetrinf->tetrode_channels[t] = new int[4];
		for (int c = 0; c < 4; ++c) {
			tetrinf->tetrode_channels[t][c] = (from+t)*4 + c;
		}
	}

	return tetrinf;
}

TetrodesInfo* TetrodesInfo::GetMergedTetrodesInfo(const TetrodesInfo* ti1, const TetrodesInfo* ti2){
	TetrodesInfo * tetrinf = new TetrodesInfo();

	const unsigned int tetrn = ti1->tetrodes_number + ti2->tetrodes_number;
	tetrinf->tetrodes_number = tetrn;

	tetrinf->channels_numbers = new int[tetrn];
	tetrinf->tetrode_channels = new int*[tetrn];

	for (int t = 0; t < ti1->tetrodes_number; ++t) {
		tetrinf->channels_numbers[t] = ti1->channels_numbers[t];
		tetrinf->tetrode_channels[t] = new int [ti1->channels_numbers[t]];
		memcpy(tetrinf->tetrode_channels[t], ti1->tetrode_channels[t], sizeof(int) * ti1->channels_numbers[t]);
	}

	const unsigned int shift = ti1->tetrodes_number;
	for (int t = 0; t < ti2->tetrodes_number; ++t) {
			tetrinf->channels_numbers[t + shift] = ti2->channels_numbers[t];
			tetrinf->tetrode_channels[t + shift] = new int [ti2->channels_numbers[t]];
			memcpy(tetrinf->tetrode_channels[t + shift], ti2->tetrode_channels[t], sizeof(int) * ti2->channels_numbers[t]);
	}

	return tetrinf;
}

TetrodesInfo::TetrodesInfo(std::string config_path) {
	std::cout << "Read tetrodes configuration from " << config_path << "\n";

	std::ifstream tconfig(config_path);

	tconfig >> tetrodes_number;

	if (tetrodes_number <= 0){
		std::cout << "# of tetrodes should be positive! Terminating...\n";
		exit(1);
	}

	channels_numbers = new int[tetrodes_number];
	tetrode_channels = new int*[tetrodes_number];

	int chnum;
	for (int t = 0; t < tetrodes_number; ++t) {
		tconfig >> chnum;
		channels_numbers[t] = chnum;
		tetrode_channels[t] = new int[chnum];
		for(int c=0; c < chnum; ++c){
			tconfig >> tetrode_channels[t][c];
		}
	}

	tconfig.close();

	tetrode_label_map_.resize(16, INVALID_TETRODE);
	for (int t = 0; t < tetrodes_number; ++t) {
		int abstetr = (int)floor(tetrode_channels[t][0] / 4);
		tetrode_label_map_[abstetr] = t;
	}
}

TetrodesInfo::TetrodesInfo() {
}

TetrodesInfo::~TetrodesInfo() {
	delete[] tetrode_by_channel;
	delete[] channels_numbers;
	for (int i = 0; i < tetrodes_number; ++i){
		delete[] tetrode_channels[i];
	}
}

bool TetrodesInfo::ContainsChannel(const unsigned int& channel) {
	for (int t = 0; t < tetrodes_number; ++t) {
		for (int c = 0; c < channels_numbers[t]; ++c) {
			if (tetrode_channels[t][c] == channel)
				return true;
		}
	}

	return false;
}

bool TetrodesInfo::ContainsChannels(const std::vector<unsigned int>& channels) {
	for (int c = 0; c < channels.size(); ++c) {
		if (!ContainsChannel(c))
			return false;
	}
	return true;
}

const int* TetrodesInfo::pc_per_chan = new int[5] {0, 5, 4, 4, 3 };

unsigned int TetrodesInfo::Translate(const TetrodesInfo* const ti, const unsigned int& tetr) {
	int abstetr = (int)floor(ti->tetrode_channels[tetr][0] / 4);
	return tetrode_label_map_[abstetr];
}
