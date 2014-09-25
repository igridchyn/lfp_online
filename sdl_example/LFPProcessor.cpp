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

#ifdef _WIN32
void usleep(__int64 usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}
#endif // _WIN32

//const int LFPBuffer::CH_MAP_INV[] = 

//const int LFPBuffer::CH_MAP[] 

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

// TODO finish implementation and use if deleting/ creating is too slow
void Spike::init(int pkg_id, int tetrode) {
	pkg_id_ = pkg_id;
	tetrode_ = tetrode;
}

Spike::~Spike() {
	for (int c = 0; c < num_channels_; ++c) {
		if (waveshape && waveshape[c])
		delete[] waveshape[c];

		if (waveshape_final && waveshape_final[c])
			delete[] waveshape_final[c];

		if (pc && pc[c])
			delete[] pc[c];
	}

	if (waveshape)
		delete[] waveshape;

	if (waveshape_final)
		delete[] waveshape_final;

	if (pc)
		delete[] pc;
}

// !!! TODO: INTRODUCE PALETTE WITH LARGER NUMBER OF COLOURS (but categorical)
const ColorPalette ColorPalette::BrewerPalette12(20, new int[20]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928, 0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00});

const ColorPalette ColorPalette::MatlabJet256(256, new int[256] {0x83, 0x87, 0x8b, 0x8f, 0x93, 0x97, 0x9b, 0x9f, 0xa3, 0xa7, 0xab, 0xaf, 0xb3, 0xb7, 0xbb, 0xbf, 0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7, 0xdb, 0xdf, 0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff, 0x3ff, 0x7ff, 0xbff, 0xfff, 0x13ff, 0x17ff, 0x1bff, 0x1fff, 0x23ff, 0x27ff, 0x2bff, 0x2fff, 0x33ff, 0x37ff, 0x3bff, 0x3fff, 0x43ff, 0x47ff, 0x4bff, 0x4fff, 0x53ff, 0x57ff, 0x5bff, 0x5fff, 0x63ff, 0x67ff, 0x6bff, 0x6fff, 0x73ff, 0x77ff, 0x7bff, 0x7fff, 0x83ff, 0x87ff, 0x8bff, 0x8fff, 0x93ff, 0x97ff, 0x9bff, 0x9fff, 0xa3ff, 0xa7ff, 0xabff, 0xafff, 0xb3ff, 0xb7ff, 0xbbff, 0xbfff, 0xc3ff, 0xc7ff, 0xcbff, 0xcfff, 0xd3ff, 0xd7ff, 0xdbff, 0xdfff, 0xe3ff, 0xe7ff, 0xebff, 0xefff, 0xf3ff, 0xf7ff, 0xfbff, 0xffff, 0x3fffb, 0x7fff7, 0xbfff3, 0xfffef, 0x13ffeb, 0x17ffe7, 0x1bffe3, 0x1fffdf, 0x23ffdb, 0x27ffd7, 0x2bffd3, 0x2fffcf, 0x33ffcb, 0x37ffc7, 0x3bffc3, 0x3fffbf, 0x43ffbb, 0x47ffb7, 0x4bffb3, 0x4fffaf, 0x53ffab, 0x57ffa7, 0x5bffa3, 0x5fff9f, 0x63ff9b, 0x67ff97, 0x6bff93, 0x6fff8f, 0x73ff8b, 0x77ff87, 0x7bff83, 0x7fff7f, 0x83ff7b, 0x87ff77, 0x8bff73, 0x8fff6f, 0x93ff6b, 0x97ff67, 0x9bff63, 0x9fff5f, 0xa3ff5b, 0xa7ff57, 0xabff53, 0xafff4f, 0xb3ff4b, 0xb7ff47, 0xbbff43, 0xbfff3f, 0xc3ff3b, 0xc7ff37, 0xcbff33, 0xcfff2f, 0xd3ff2b, 0xd7ff27, 0xdbff23, 0xdfff1f, 0xe3ff1b, 0xe7ff17, 0xebff13, 0xefff0f, 0xf3ff0b, 0xf7ff07, 0xfbff03, 0xffff00, 0xfffb00, 0xfff700, 0xfff300, 0xffef00, 0xffeb00, 0xffe700, 0xffe300, 0xffdf00, 0xffdb00, 0xffd700, 0xffd300, 0xffcf00, 0xffcb00, 0xffc700, 0xffc300, 0xffbf00, 0xffbb00, 0xffb700, 0xffb300, 0xffaf00, 0xffab00, 0xffa700, 0xffa300, 0xff9f00, 0xff9b00, 0xff9700, 0xff9300, 0xff8f00, 0xff8b00, 0xff8700, 0xff8300, 0xff7f00, 0xff7b00, 0xff7700, 0xff7300, 0xff6f00, 0xff6b00, 0xff6700, 0xff6300, 0xff5f00, 0xff5b00, 0xff5700, 0xff5300, 0xff4f00, 0xff4b00, 0xff4700, 0xff4300, 0xff3f00, 0xff3b00, 0xff3700, 0xff3300, 0xff2f00, 0xff2b00, 0xff2700, 0xff2300, 0xff1f00, 0xff1b00, 0xff1700, 0xff1300, 0xff0f00, 0xff0b00, 0xff0700, 0xff0300, 0xff0000, 0xfb0000, 0xf70000, 0xf30000, 0xef0000, 0xeb0000, 0xe70000, 0xe30000, 0xdf0000, 0xdb0000, 0xd70000, 0xd30000, 0xcf0000, 0xcb0000, 0xc70000, 0xc30000, 0xbf0000, 0xbb0000, 0xb70000, 0xb30000, 0xaf0000, 0xab0000, 0xa70000, 0xa30000, 0x9f0000, 0x9b0000, 0x970000, 0x930000, 0x8f0000, 0x8b0000, 0x870000, 0x830000, 0x7f0000});

// ============================================================================================================


void LFPBuffer::Reset(Config* config) {
	if (config_)
		delete config_;

	config_ = config;

	spike_buf_pos = SPIKE_BUF_HEAD_LEN;
	spike_buf_nows_pos = SPIKE_BUF_HEAD_LEN;
	spike_buf_no_rec = SPIKE_BUF_HEAD_LEN;
	spike_buf_no_disp_pca = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_unproc_ = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_out = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_clust_ = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_draw_xy = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_speed_ = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_pop_vec_ = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_pf_ = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_auto_ = SPIKE_BUF_HEAD_LEN;
	spike_buf_pos_lpt_ = SPIKE_BUF_HEAD_LEN;

	if (tetr_info_)
		delete tetr_info_;
	tetr_info_ = new TetrodesInfo(config->getString("tetr.conf.path"));

	cluster_spike_counts_ = arma::mat(tetr_info_->tetrodes_number, 40, arma::fill::zeros);
	log_stream << "Buffer reset\n";
	log_stream << "INFO: # of tetrodes: " << tetr_info_->tetrodes_number << "\n";
	log_stream << "INFO: set memory...";

	for(int c=0; c < CHANNEL_NUM; ++c){
	        memset(signal_buf[c], 0, LFP_BUF_LEN * sizeof(short));
	        memset(filtered_signal_buf[c], 0, LFP_BUF_LEN * sizeof(int));
	        memset(power_buf[c], 0, LFP_BUF_LEN * sizeof(int));
	        if (powerEstimatorsMap_[c] != NULL){
	        	delete powerEstimatorsMap_[c];
	        	powerEstimatorsMap_[c] = NULL;
	        }
	}

	log_stream << "done\n";

	log_stream << "INFO: Create online estimators...";
    powerEstimators_ = new OnlineEstimator<float>[tetr_info_->tetrodes_number];
    // TODO: configurableize
    speedEstimator_ = new OnlineEstimator<float>(16);
	log_stream << "done\n";

    memset(is_valid_channel_, 0, CHANNEL_NUM);
    tetr_info_->tetrode_by_channel = new int[CHANNEL_NUM];

    if (last_spike_pos_)
    	delete[] last_spike_pos_;
    last_spike_pos_ = new int[tetr_info_->tetrodes_number];

    // create a map of pointers to tetrode power estimators for each electrode
    for (int tetr = 0; tetr < tetr_info_->tetrodes_number; ++tetr ){
        for (int ci = 0; ci < tetr_info_->channels_numbers[tetr]; ++ci){
            powerEstimatorsMap_[tetr_info_->tetrode_channels[tetr][ci]] = powerEstimators_ + tetr;
            is_valid_channel_[tetr_info_->tetrode_channels[tetr][ci]] = true;

            tetr_info_->tetrode_by_channel[tetr_info_->tetrode_channels[tetr][ci]] = tetr;
        }
    }

    population_vector_window_.clear();
    population_vector_window_.resize(tetr_info_->tetrodes_number);
    // initialize for counting unclusterred spikes
    // clustering processors should resize this to use for clustered spikes clusters
    for (int t=0; t < tetr_info_->tetrodes_number; ++t){
    	population_vector_window_[t].push_back(0);
    }

	log_stream << "INFO: BUFFER CREATED\n";

	memset(spike_buffer_, 0, SPIKE_BUF_LEN * sizeof(Spike*));

	for (int pos_buf = 0; pos_buf < _POS_BUF_SIZE; ++pos_buf) {
		// TODO fix
		memset(positions_buf_[pos_buf], 0, 6 * sizeof(unsigned int));
	}

	ISIEstimators_ = new OnlineEstimator<float>*[tetr_info_->tetrodes_number];
	for (int t = 0; t < tetr_info_->tetrodes_number; ++t) {
		ISIEstimators_[t] = new OnlineEstimator<float>();
	}

	previous_spikes_pkg_ids_ = new unsigned int[tetr_info_->tetrodes_number];
	// fill with fake spikes to avoid unnnecessery checks because of the first time
	// (memory for 1 spike per tetrode will be lost)
	// TODO fix for package IDs starting not with null
	// TODO warn packages strating not with 0
	for (int t = 0; t < tetr_info_->tetrodes_number; ++t) {
		previous_spikes_pkg_ids_[t] = 0;
	}

	is_high_synchrony_tetrode_ = new bool[tetr_info_->tetrodes_number];
	memset(is_high_synchrony_tetrode_, 0, sizeof(bool) * tetr_info_->tetrodes_number);
	for (int t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
		is_high_synchrony_tetrode_[config_->synchrony_tetrodes_[t]] =  true;
	}

	// TODO check if all members are properly being reset
	high_synchrony_tetrode_spikes_ = 0;
}

LFPBuffer::~LFPBuffer(){
	Log("Buffer destructor called");
}

LFPBuffer::LFPBuffer(Config* config)
: POP_VEC_WIN_LEN(config->getInt("pop.vec.win.len.ms"))
, SAMPLING_RATE(config->getInt("sampling.rate"))
, pos_unknown_(config->getInt("pos.unknown", 1023))
, SPIKE_BUF_LEN(config->getInt("spike.buf.size", 1 << 24))
, SPIKE_BUF_HEAD_LEN(config->getInt("spike.buf.head", 1 << 18))
, LFP_BUF_LEN(config->getInt("buf.len", 1 << 11))
, BUF_HEAD_LEN(config->getInt("buf.head.len", 1 << 8))
, high_synchrony_factor_(config->getFloat("high.synchrony.factor", 2.0f))
{
	CH_MAP = new int[64]{8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31, 40, 41, 42, 43, 44, 45, 46, 47, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 32, 33, 34, 35, 36, 37, 38, 39, 48, 49, 50, 51, 52, 53, 54, 55};

	// OLD
	//CH_MAP_INV = new int[64]{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 42, 43, 44, 45, 46, 47, 8, 9, 10, 11, 12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 16, 17, 18, 19, 20, 21, 22, 23, 56, 57, 58, 59, 60, 61, 62, 63, 24, 25, 26, 27, 28, 29, 30, 31 };
	//CH_MAP_INV = new int[64]{48,49,50,51,52,53,54,55,32,33,34,35,36,37,38,39,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7,56,57,58,59,60,61,62,63,40,41,42,43,44,45,46,47,24,25,26,27,28,29,30,31,8,9,10,11,12,13,14,15};
	CH_MAP_INV = new int[64]{8,9,10,11,12,13,14,15,24,25,26,27,28,29,30,31,40,41,42,43,44,45,46,47,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23,32,33,34,35,36,37,38,39,48,49,50,51,52,53,54,55};

	for (size_t i = 0; i < CHANNEL_NUM; i++)
	{
		powerEstimatorsMap_[i] = NULL;
	}

	spike_buffer_ = new Spike*[SPIKE_BUF_LEN];

	// TODO !!! creatre normal random name
	
	srand(time(NULL));
	int i = rand() % 64;
	std::string log_path_prefix = config->getString("log.path.prefix", "lfponline_LOG_");
	log_stream.open(log_path_prefix + Utils::NUMBERS[i] + ".txt", std::ios_base::app);
	std::cout << "Created LOG\n";

    for (int c = 0; c < CHANNEL_NUM; ++c){
    	unsigned int WS_SHIFT = 100;
    	signal_buf[c] = new short[LFP_BUF_LEN + WS_SHIFT];
    	// + waveshape length
    	filtered_signal_buf[c] = new int[LFP_BUF_LEN + WS_SHIFT];
    	power_buf[c] = new int[LFP_BUF_LEN + WS_SHIFT];
    }

    Reset(config);
}

// pop spikes from the top of the queue who fall outside of population window of given length ending in last known PKG_ID
void LFPBuffer::RemoveSpikesOutsideWindow(const unsigned int& right_border){
    if (population_vector_stack_.empty()){
        return;
    }
    
    Spike *stop = population_vector_stack_.front();
    while (stop->pkg_id_ < right_border - POP_VEC_WIN_LEN * SAMPLING_RATE / 1000.f) {
    	// this duplicates population_vector_window_ + config->synchrony tetrodes, but is needed for efficiency ...
    	if (is_high_synchrony_tetrode_[stop->tetrode_])
    		high_synchrony_tetrode_spikes_ --;

        population_vector_stack_.pop();
        
        population_vector_window_[stop->tetrode_][stop->cluster_id_ + 1] --;
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
    
    population_vector_window_[spike->tetrode_][spike->cluster_id_ + 1] ++;
    population_vector_total_spikes_ ++;
    
    population_vector_stack_.push(spike);

    // TODO separate processors for ISI estimation
    if (ISIEstimators_ != NULL){
    	ISIEstimators_[spike->tetrode_]->push((spike->pkg_id_ - previous_spikes_pkg_ids_[spike->tetrode_]) / (float)SAMPLING_RATE);

    	// ??? do outside if prev_spikes used anywhere else
    	previous_spikes_pkg_ids_[spike->tetrode_] = spike->pkg_id_;
    }

    if (is_high_synchrony_tetrode_[spike->tetrode_])
    	high_synchrony_tetrode_spikes_ ++;

    // DEBUG - print pop vector occasionally
//    if (!(spike->pkg_id_ % 3000)){
//        std::cout << "Pop. vector: \n";
//        for (int t=0; t < tetr_info_->tetrodes_number; ++t) {
//            std::cout << "\t";
//            for (int c=0; c < population_vector_window_[t].size(); ++c) {
//                std::cout << population_vector_window_[t][c] << " ";
//            }
//            std::cout << "\n";
//        }
//    }
}


void LFPBuffer::AddSpike(Spike* spike) {
	spike_buffer_[spike_buf_pos] = spike;
	spike_buf_pos++;

	// check if rewind is requried
	if (spike_buf_pos == SPIKE_BUF_LEN - 1){
		memcpy(spike_buffer_, spike_buffer_ + spike_buf_pos - SPIKE_BUF_HEAD_LEN, sizeof(Spike*)*SPIKE_BUF_HEAD_LEN);
		//                    for (int del_spike = SPIKE_BUF_HEAD_LEN; del_spike < spike_buf_pos; ++del_spike) {
		//                    	delete spike_buffer_[del_spike];
		//                    	spike_buffer_[del_spike] = NULL;
		//					}

		const int shift_new_start = spike_buf_pos - SPIKE_BUF_HEAD_LEN;

		spike_buf_no_rec -= std::min(shift_new_start, (int)spike_buf_no_rec);
		spike_buf_nows_pos -= std::min(shift_new_start, (int)spike_buf_nows_pos);
		spike_buf_pos_unproc_ -= std::min(shift_new_start, (int)spike_buf_pos_unproc_);
		spike_buf_no_disp_pca -= std::min(shift_new_start, (int)spike_buf_no_disp_pca );
		spike_buf_pos_out -= std::min(shift_new_start, (int)spike_buf_pos_out );
		spike_buf_pos_draw_xy -= std::min(shift_new_start, (int)spike_buf_pos_draw_xy );
		spike_buf_pos_speed_ -= std::min(shift_new_start, (int)spike_buf_pos_speed_);
		spike_buf_pos_pop_vec_ -= std::min(shift_new_start, (int)spike_buf_pos_pop_vec_);
		spike_buf_pos_clust_ -= std::min(shift_new_start, (int)spike_buf_pos_clust_);
		spike_buf_pos_pf_ -= std::min(shift_new_start, (int)spike_buf_pos_pf_);
		spike_buf_pos_auto_ -= std::min(shift_new_start, (int)spike_buf_pos_auto_);
		spike_buf_pos_lpt_ -= std::min(shift_new_start, (int)spike_buf_pos_lpt_);

		spike_buf_pos = SPIKE_BUF_HEAD_LEN;

		std::cout << "Spike buffer rewind (at pos " << buf_pos <<  ")!\n";
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

void LFPBuffer::Log(std::string message) {
	std::cout << message << "\n";
	log_stream << message << "\n";
	// TODO remove in release
	log_stream.flush();
}

void LFPProcessor::Log(std::string message) {
	buffer->Log(name() + ": " + message);
}

std::string LFPProcessor::name() {
	return "<processor name not specified>";
}

// whether current population window represents high synchrony activity
bool LFPBuffer::IsHighSynchrony() {
	RemoveSpikesOutsideWindow(last_pkg_id);

	// whether have at least synchrony.factor X average spikes at all tetrodes
	float average_spikes_window = .0f;
	unsigned int spikes_pop_synchrony= 0;

	for (int t = 0; t < config_->synchrony_tetrodes_.size(); ++t) {
		average_spikes_window += 1.0 / ISIEstimators_[config_->synchrony_tetrodes_[t]]->get_mean_estimate();
	}
	average_spikes_window *= POP_VEC_WIN_LEN / 1000.0f;

	return (high_synchrony_tetrode_spikes_ >= average_spikes_window * high_synchrony_factor_);
}

void LFPBuffer::Log(std::string message, int num) {
	std::cout << message << num << "\n";
	log_stream << message << num << "\n";
	// TODO remove in release
	log_stream.flush();
}

void LFPProcessor::Log(std::string message, int num) {
	buffer->Log(name() + ": " + message, num);
}
