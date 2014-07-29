//
//  LFPProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 31/03/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "OnlineEstimator.cpp"
#include <fstream>
#include <assert.h>
#include "SDL2/SDL.h"

// REQUIREMENTS AND SPECIFICATION
// data structure for LFP information (for all processors)
// data incoming for processing in BATCHES (of variable size, potentially)
// channel processor in a separate THREAD
// higher level processors have to combine information accross channels
// large amount of data to be stored:
//      write to file / use buffer for last events ( for batch writing )

// OPTIMIZE
//  iteration over packages in LFPBuffer - create buffer iterator TILL last package

// ???
// do all processors know information about subsequent structure?
//  NO
// put spikes into buffer ? (has to be available later)



const int LFPBuffer::CH_MAP_INV[] = {8,9,10,11,12,13,14,15,24,25,26,27,28,29,30,31,40,41,42,43,44,45,46,47,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23,32,33,34,35,36,37,38,39,48,49,50,51,52,53,54,55};

const int LFPBuffer::CH_MAP[] = {32, 33, 34, 35, 36, 37, 38, 39,0, 1, 2, 3, 4, 5, 6, 7,40, 41, 42, 43, 44, 45, 46, 47,8, 9, 10, 11, 12, 13, 14, 15,48, 49, 50, 51, 52, 53, 54, 55,16, 17, 18, 19, 20, 21, 22, 23,56, 57, 58, 59, 60, 61, 62, 63,24, 25, 26, 27, 28, 29, 30, 31};

// ============================================================================================================

int TetrodesInfo::number_of_channels(Spike *spike){
    return channels_numbers[spike->tetrode_];
}

// ============================================================================================================



// ============================================================================================================

void FileOutputProcessor::process(){
    while (buffer->spike_buf_pos_out < buffer->spike_buf_pos_unproc_){
        Spike* spike = buffer->spike_buffer_[buffer->spike_buf_pos_out];
        if (spike->discarded_){
            buffer->spike_buf_pos_out++;
            continue;
        }
        
        for(int c=0;c<4;++c){
            for(int w=0; w<16; ++w){
                fprintf(f_, "%d ", spike->waveshape_final[c][w]);
            }
            fprintf(f_, "\n");
        }
        
        buffer->spike_buf_pos_out++;
    }
}

FileOutputProcessor::~FileOutputProcessor(){
    fclose(f_);
}

// ============================================================================================================

Spike::Spike(int pkg_id, int tetrode)
    : pkg_id_(pkg_id)
    , tetrode_(tetrode)
{
}

// !!! TODO: INTRODUCE PALETTE WITH LARGER NUMBER OF COLOURS (but categorical)
const ColorPalette ColorPalette::BrewerPalette12(20, new int[20]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928, 0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00});

const ColorPalette ColorPalette::MatlabJet256(256, new int[256] {0x83, 0x87, 0x8b, 0x8f, 0x93, 0x97, 0x9b, 0x9f, 0xa3, 0xa7, 0xab, 0xaf, 0xb3, 0xb7, 0xbb, 0xbf, 0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7, 0xdb, 0xdf, 0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff, 0x3ff, 0x7ff, 0xbff, 0xfff, 0x13ff, 0x17ff, 0x1bff, 0x1fff, 0x23ff, 0x27ff, 0x2bff, 0x2fff, 0x33ff, 0x37ff, 0x3bff, 0x3fff, 0x43ff, 0x47ff, 0x4bff, 0x4fff, 0x53ff, 0x57ff, 0x5bff, 0x5fff, 0x63ff, 0x67ff, 0x6bff, 0x6fff, 0x73ff, 0x77ff, 0x7bff, 0x7fff, 0x83ff, 0x87ff, 0x8bff, 0x8fff, 0x93ff, 0x97ff, 0x9bff, 0x9fff, 0xa3ff, 0xa7ff, 0xabff, 0xafff, 0xb3ff, 0xb7ff, 0xbbff, 0xbfff, 0xc3ff, 0xc7ff, 0xcbff, 0xcfff, 0xd3ff, 0xd7ff, 0xdbff, 0xdfff, 0xe3ff, 0xe7ff, 0xebff, 0xefff, 0xf3ff, 0xf7ff, 0xfbff, 0xffff, 0x3fffb, 0x7fff7, 0xbfff3, 0xfffef, 0x13ffeb, 0x17ffe7, 0x1bffe3, 0x1fffdf, 0x23ffdb, 0x27ffd7, 0x2bffd3, 0x2fffcf, 0x33ffcb, 0x37ffc7, 0x3bffc3, 0x3fffbf, 0x43ffbb, 0x47ffb7, 0x4bffb3, 0x4fffaf, 0x53ffab, 0x57ffa7, 0x5bffa3, 0x5fff9f, 0x63ff9b, 0x67ff97, 0x6bff93, 0x6fff8f, 0x73ff8b, 0x77ff87, 0x7bff83, 0x7fff7f, 0x83ff7b, 0x87ff77, 0x8bff73, 0x8fff6f, 0x93ff6b, 0x97ff67, 0x9bff63, 0x9fff5f, 0xa3ff5b, 0xa7ff57, 0xabff53, 0xafff4f, 0xb3ff4b, 0xb7ff47, 0xbbff43, 0xbfff3f, 0xc3ff3b, 0xc7ff37, 0xcbff33, 0xcfff2f, 0xd3ff2b, 0xd7ff27, 0xdbff23, 0xdfff1f, 0xe3ff1b, 0xe7ff17, 0xebff13, 0xefff0f, 0xf3ff0b, 0xf7ff07, 0xfbff03, 0xffff00, 0xfffb00, 0xfff700, 0xfff300, 0xffef00, 0xffeb00, 0xffe700, 0xffe300, 0xffdf00, 0xffdb00, 0xffd700, 0xffd300, 0xffcf00, 0xffcb00, 0xffc700, 0xffc300, 0xffbf00, 0xffbb00, 0xffb700, 0xffb300, 0xffaf00, 0xffab00, 0xffa700, 0xffa300, 0xff9f00, 0xff9b00, 0xff9700, 0xff9300, 0xff8f00, 0xff8b00, 0xff8700, 0xff8300, 0xff7f00, 0xff7b00, 0xff7700, 0xff7300, 0xff6f00, 0xff6b00, 0xff6700, 0xff6300, 0xff5f00, 0xff5b00, 0xff5700, 0xff5300, 0xff4f00, 0xff4b00, 0xff4700, 0xff4300, 0xff3f00, 0xff3b00, 0xff3700, 0xff3300, 0xff2f00, 0xff2b00, 0xff2700, 0xff2300, 0xff1f00, 0xff1b00, 0xff1700, 0xff1300, 0xff0f00, 0xff0b00, 0xff0700, 0xff0300, 0xff0000, 0xfb0000, 0xf70000, 0xf30000, 0xef0000, 0xeb0000, 0xe70000, 0xe30000, 0xdf0000, 0xdb0000, 0xd70000, 0xd30000, 0xcf0000, 0xcb0000, 0xc70000, 0xc30000, 0xbf0000, 0xbb0000, 0xb70000, 0xb30000, 0xaf0000, 0xab0000, 0xa70000, 0xa30000, 0x9f0000, 0x9b0000, 0x970000, 0x930000, 0x8f0000, 0x8b0000, 0x870000, 0x830000, 0x7f0000});

// ============================================================================================================

LFPBuffer::LFPBuffer(TetrodesInfo* tetr_info, Config* config)
: tetr_info_(tetr_info)
, POP_VEC_WIN_LEN(config->getInt("pop.vec.win.len.ms"))
, cluster_spike_counts_(tetr_info->tetrodes_number, 40, arma::fill::zeros)
, SAMPLING_RATE(config->getInt("sampling.rate"))
, config_(config){
    
    for(int c=0; c < CHANNEL_NUM; ++c){
        memset(signal_buf[c], LFP_BUF_LEN, sizeof(int));
        memset(filtered_signal_buf[c], LFP_BUF_LEN, sizeof(int));
        memset(power_buf[c], LFP_BUF_LEN, sizeof(int));
        powerEstimatorsMap_[c] = NULL;
    }
    
    powerEstimators_ = new OnlineEstimator<float>[tetr_info_->tetrodes_number];
    // TODO: configurableize
    speedEstimator_ = new OnlineEstimator<float>(16);
    
    tetr_info_->tetrode_by_channel = new int[CHANNEL_NUM];
    
    // create a map of pointers to tetrode power estimators for each electrode
    for (int tetr = 0; tetr < tetr_info_->tetrodes_number; ++tetr ){
        for (int ci = 0; ci < tetr_info_->channels_numbers[tetr]; ++ci){
            powerEstimatorsMap_[tetr_info_->tetrode_channels[tetr][ci]] = powerEstimators_ + tetr;
            is_valid_channel_[tetr_info_->tetrode_channels[tetr][ci]] = true;
            
            tetr_info_->tetrode_by_channel[tetr_info_->tetrode_channels[tetr][ci]] = tetr;
        }
    }
    
    last_spike_pos_ = new int[tetr_info_->tetrodes_number];
    
    population_vector_window_.resize(tetr_info_->tetrodes_number);
}

void LFPBuffer::RemoveSpikesOutsideWindow(const unsigned int& right_border){
    if (population_vector_stack_.empty()){
        return;
    }
    
    Spike *stop = population_vector_stack_.front();
    while (stop->pkg_id_ < right_border - POP_VEC_WIN_LEN * SAMPLING_RATE / 1000.f) {
        population_vector_stack_.pop();
        
        population_vector_window_[stop->tetrode_][stop->cluster_id_] --;
        population_vector_total_spikes_ --;
        
        if (population_vector_stack_.empty()){
            break;
        }
        
        stop = population_vector_stack_.front();
    }
}

void LFPBuffer::UpdateWindowVector(Spike *spike){
    // TODO: make right border of the window more precise: as close as possible to lst_pkg_id but not containing unclassified spikes
    // (left border = right border - POP_VEC_WIN_LEN)
    
    population_vector_window_[spike->tetrode_][spike->cluster_id_] ++;
    population_vector_total_spikes_ ++;
    
    population_vector_stack_.push(spike);
    
    // DEBUG - print pop vector occasionally
    if (!(spike->pkg_id_ % 3000)){
        std::cout << "Pop. vector: \n";
        for (int t=0; t < tetr_info_->tetrodes_number; ++t) {
            std::cout << "\t";
            for (int c=0; c < population_vector_window_[t].size(); ++c) {
                std::cout << population_vector_window_[t][c] << " ";
            }
            std::cout << "\n";
        }
    }
}

// ============================================================================================================


ColorPalette::ColorPalette(int num_colors, int *color_values)
    : num_colors_(num_colors)
, color_values_(color_values) {}

int ColorPalette::getR(int order) const{
    return (color_values_[order] & 0xFF0000) >> 16;
}
int ColorPalette::getG(int order) const{
    return (color_values_[order] & 0x00FF00) >> 8;
}
int ColorPalette::getB(int order) const{
    return color_values_[order] & 0x0000FF;
}
int ColorPalette::getColor(int order) const{
    return color_values_[order];
}

const char * const Utils::NUMBERS[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61", "62", "63"};

std::string Utils::Converter::int2str(int a){
	if (a == 0)
		return "0";

	std::string s = "";

	while(a > 0){
		s = std::string(Utils::NUMBERS[a % 10]) + s;
		a = a / 10;
	}

	return s;
}

// TODO: fix types
std::vector<int> Utils::Math::GetRange(const unsigned int& from, const unsigned int& to){
	std::vector<int> range;
	for (int i = 0; i < to-from+1; ++i) {
		range.push_back(from + i);
	}
	return range;
}

// TODO: fix types
std::vector<int> Utils::Math::MergeRanges(const std::vector<int>& a1, const std::vector<int>& a2){
	std::vector<int> merged;
	merged = a1;

	for (int i = 0; i < a2.size(); ++i) {
		merged.push_back(a2[i]);
	}

	return merged;
}

void Utils::Output::printIntArray(int *array, const unsigned int num_el){
	for (int e = 0; e < num_el; ++e) {
		std::cout << array[e] << " ";
	}
	std::cout << "\n";
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
}

TetrodesInfo::TetrodesInfo() {
}
