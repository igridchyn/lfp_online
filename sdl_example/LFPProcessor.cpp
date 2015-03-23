//
//  LFPProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 31/03/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "OnlineEstimator.cpp"
#include <assert.h>

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

	timer = CreateWaitableTimer(nullptr, TRUE, nullptr);
	SetWaitableTimer(timer, &ft, 0, nullptr, nullptr, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}
#endif // _WIN32

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
//	extra_features_ = new float*[4] {&peak_to_valley_1_, &peak_to_valley_2_, &intervalley_, &power_};
}

void Spike::init(int pkg_id, int tetrode) {
	// initial values reset (instead of the array memory, which is managed by processors and buffer

	pkg_id_ = pkg_id;
	tetrode_ = tetrode;

	peak_to_valley_1_ = peak_to_valley_2_ = intervalley_ = power_ = .0f;
	cluster_id_ = num_channels_ = -1;

	discarded_ = false;
	aligned_ = false;

	x =y = speed = nanf("");

	peak_chan_ = 255;

	num_pc_ = 0;
}

float Spike::getWidth(float level, int chan) {
	if (waveshape == nullptr)
		return 0;

	// find first crossing
	bool reached_val = false;
	double level_x = .0;

	// compute width
	// TODO validate > or <
	for (int w = 1; w < 128 - 1; ++w) {
		if (! reached_val && waveshape[chan][w] > level){
			reached_val = true;
			level_x = w - 1 + (level - waveshape[chan][w - 1]) / (waveshape[chan][w] - waveshape[chan][w - 1]);
		}

		if (reached_val && waveshape[chan][w] < level){
			double level_x_down = w - 1 + (level - waveshape[chan][w - 1]) / (waveshape[chan][w] - waveshape[chan][w - 1]);
			return level_x_down - level_x;
		}
	}

	if (!reached_val)
		return 0;

	return 0;
}

Spike::~Spike() {
//	for (int c = 0; c < num_channels_; ++c) {
//		if (waveshape && waveshape[c])
//		delete[] waveshape[c];

//		if (waveshape_final && waveshape_final[c])
//			delete[] waveshape_final[c];
//	}

//	if (pc)
//		delete[] pc;

//	if (waveshape)
//		delete[] waveshape;

//	if (waveshape_final)
//		delete[] waveshape_final;

//	delete[] extra_features_;
}

// !!! TODO: INTRODUCE PALETTE WITH LARGER NUMBER OF COLOURS (but categorical)
const ColorPalette ColorPalette::BrewerPalette12(20, new int[20]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928, 0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00});

const ColorPalette ColorPalette::MatlabJet256(256, new int[256] {0x83, 0x87, 0x8b, 0x8f, 0x93, 0x97, 0x9b, 0x9f, 0xa3, 0xa7, 0xab, 0xaf, 0xb3, 0xb7, 0xbb, 0xbf, 0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7, 0xdb, 0xdf, 0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff, 0x3ff, 0x7ff, 0xbff, 0xfff, 0x13ff, 0x17ff, 0x1bff, 0x1fff, 0x23ff, 0x27ff, 0x2bff, 0x2fff, 0x33ff, 0x37ff, 0x3bff, 0x3fff, 0x43ff, 0x47ff, 0x4bff, 0x4fff, 0x53ff, 0x57ff, 0x5bff, 0x5fff, 0x63ff, 0x67ff, 0x6bff, 0x6fff, 0x73ff, 0x77ff, 0x7bff, 0x7fff, 0x83ff, 0x87ff, 0x8bff, 0x8fff, 0x93ff, 0x97ff, 0x9bff, 0x9fff, 0xa3ff, 0xa7ff, 0xabff, 0xafff, 0xb3ff, 0xb7ff, 0xbbff, 0xbfff, 0xc3ff, 0xc7ff, 0xcbff, 0xcfff, 0xd3ff, 0xd7ff, 0xdbff, 0xdfff, 0xe3ff, 0xe7ff, 0xebff, 0xefff, 0xf3ff, 0xf7ff, 0xfbff, 0xffff, 0x3fffb, 0x7fff7, 0xbfff3, 0xfffef, 0x13ffeb, 0x17ffe7, 0x1bffe3, 0x1fffdf, 0x23ffdb, 0x27ffd7, 0x2bffd3, 0x2fffcf, 0x33ffcb, 0x37ffc7, 0x3bffc3, 0x3fffbf, 0x43ffbb, 0x47ffb7, 0x4bffb3, 0x4fffaf, 0x53ffab, 0x57ffa7, 0x5bffa3, 0x5fff9f, 0x63ff9b, 0x67ff97, 0x6bff93, 0x6fff8f, 0x73ff8b, 0x77ff87, 0x7bff83, 0x7fff7f, 0x83ff7b, 0x87ff77, 0x8bff73, 0x8fff6f, 0x93ff6b, 0x97ff67, 0x9bff63, 0x9fff5f, 0xa3ff5b, 0xa7ff57, 0xabff53, 0xafff4f, 0xb3ff4b, 0xb7ff47, 0xbbff43, 0xbfff3f, 0xc3ff3b, 0xc7ff37, 0xcbff33, 0xcfff2f, 0xd3ff2b, 0xd7ff27, 0xdbff23, 0xdfff1f, 0xe3ff1b, 0xe7ff17, 0xebff13, 0xefff0f, 0xf3ff0b, 0xf7ff07, 0xfbff03, 0xffff00, 0xfffb00, 0xfff700, 0xfff300, 0xffef00, 0xffeb00, 0xffe700, 0xffe300, 0xffdf00, 0xffdb00, 0xffd700, 0xffd300, 0xffcf00, 0xffcb00, 0xffc700, 0xffc300, 0xffbf00, 0xffbb00, 0xffb700, 0xffb300, 0xffaf00, 0xffab00, 0xffa700, 0xffa300, 0xff9f00, 0xff9b00, 0xff9700, 0xff9300, 0xff8f00, 0xff8b00, 0xff8700, 0xff8300, 0xff7f00, 0xff7b00, 0xff7700, 0xff7300, 0xff6f00, 0xff6b00, 0xff6700, 0xff6300, 0xff5f00, 0xff5b00, 0xff5700, 0xff5300, 0xff4f00, 0xff4b00, 0xff4700, 0xff4300, 0xff3f00, 0xff3b00, 0xff3700, 0xff3300, 0xff2f00, 0xff2b00, 0xff2700, 0xff2300, 0xff1f00, 0xff1b00, 0xff1700, 0xff1300, 0xff0f00, 0xff0b00, 0xff0700, 0xff0300, 0xff0000, 0xfb0000, 0xf70000, 0xf30000, 0xef0000, 0xeb0000, 0xe70000, 0xe30000, 0xdf0000, 0xdb0000, 0xd70000, 0xd30000, 0xcf0000, 0xcb0000, 0xc70000, 0xc30000, 0xbf0000, 0xbb0000, 0xb70000, 0xb30000, 0xaf0000, 0xab0000, 0xa70000, 0xa30000, 0x9f0000, 0x9b0000, 0x970000, 0x930000, 0x8f0000, 0x8b0000, 0x870000, 0x830000, 0x7f0000});

// ============================================================================================================


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

void LFPProcessor::Log(std::string message) {
	buffer->Log(name() + ": " + message);
}

void LFPProcessor::Log(std::string message, unsigned int num) {
	buffer->Log(name() + ": " + message + Utils::Converter::int2str(num));
}

int LFPProcessor::getInt(std::string name) {
	int p0val = buffer->config_->getInt(name);

	if (processor_number_ > 0){
		return buffer->config_->getInt(name + "." + Utils::NUMBERS[processor_number_], p0val);
	}
	else{
		return p0val;
	}
}

std::string LFPProcessor::getString(std::string name) {
	std::string p0val = buffer->config_->getString(name);

	if (processor_number_ > 0){
		return buffer->config_->getString(name + "." + Utils::NUMBERS[processor_number_], p0val);
	}
	else{
		return p0val;
	}
}

bool LFPProcessor::getBool(std::string name) {
	bool p0val = buffer->config_->getBool(name);

	if (processor_number_ > 0){
		return buffer->config_->getBool(name + "." + Utils::NUMBERS[processor_number_], p0val);
	}
	else{
		return p0val;
	}
}

float LFPProcessor::getFloat(std::string name) {
	float p0val = buffer->config_->getFloat(name);

	if (processor_number_ > 0){
		return buffer->config_->getFloat(name + "." + Utils::NUMBERS[processor_number_], p0val);
	}
	else{
		return p0val;
	}
}

std::string LFPProcessor::getOutPath(std::string name) {
	std::string p0val = buffer->config_->getOutPath(name);

	if (processor_number_ > 0){
		return buffer->config_->getOutPath(name + "." + Utils::NUMBERS[processor_number_], p0val);
	}
	else{
		return p0val;
	}
}

std::string LFPProcessor::name() {
	return "<processor name not specified>";
}


void LFPProcessor::Log(std::string message, int num) {
	buffer->Log(name() + ": " + message, num);
}

void LFPProcessor::Log(std::string message, double num) {
	buffer->Log(name() + ": " + message, num);
}

// if point (x3, y3) is to the right from vector (x1, y2)->(x2, y2)
bool IsFromRightWave(float x1, float y1, float x2, float y2, float x3, float y3){

	// edge vector rotated 90 clock-wise
	float ex90 = y2 - y1;
	float ey90 = - (x2 - x1);

	float px = x3 - x1;
	float py = y3 - y1;

	// cos of angle between edge vector and vector from first vertex to test point
	float cossig = px * ex90 + py * ey90;

	return cossig < 0;
}

bool Spike::crossesWaveShapeFinal(unsigned int channel, int x1, int y1, int x2, int y2) {

	int w1 = floor(x1);
	int w2 = floor(x2);

	// TODO parametrize
	if (w1 > 16 || w2 > 16)
		return false;

	for (int c = 0; c < num_channels_; ++c) {
		bool down1 = IsFromRightWave(w1, waveshape_final[c][w1], w1 + 1, waveshape_final[c][w1 + 1], x1, y1);
		bool down2 = IsFromRightWave(w2, waveshape_final[c][w2], w2 + 1, waveshape_final[c][w2 + 1], x2, y2);

		if (down1 ^ down2)
			return true;
	}

	return false;
}

bool Spike::crossesWaveShapeReconstructed(unsigned int channel, int x1, int y1,
		int x2, int y2) {
	int w1 = floor(x1);
	int w2 = floor(x2);

	// TODO parametrize
	if (w1 > 128 || w2 > 128)
		return false;

	bool down1 = IsFromRightWave(w1, waveshape[channel][w1], w1 + 1, waveshape[channel][w1 + 1], x1, y1);

	for (int w = w1 + 1; w <= w2; ++w){
		if (down1 ^ !IsFromRightWave(x1, y1, x2, y2, w, waveshape[channel][w]))
			return true;
	}


	return false;
}

void Spike::find_one_peak(int* ptmout, int peakp, int peakgit, int* ptmval) {
    int pmax,ptm,i,j;
    // !!! why channel 0 ???
    pmax=waveshape[0][peakp];
    ptm=peakp;

    for(i=0;i < num_channels_;i++) {
        for(j=peakp-peakgit;j<peakp+peakgit;j++)  {
            if (j<0) continue;
            if (waveshape[i][j] < pmax){
                ptm=j;
                pmax=waveshape[i][j];
            }
        }
    }
    *ptmout=ptm;
    *ptmval=pmax;
}

void Spike::find_valleys(int ptm, int ptv, float *valley_time_1, float *valley_time_2, float *intervalley)
{
  int **avb = waveshape;
  // TODO configurableize
  int tmbefsp = 7;
  int tmaftsp = 7;

  int i,j,pm1,pm2,tma,tmb;
  pm1=pm2=avb[0][ptm];
  tma=tmb=ptm;
  for(i=0; i<num_channels_; i++) {
	  for(j=ptm - tmbefsp; j < ptm; j++) {
		  if (j<0) continue;
		  if (avb[i][j]>pm1) {
			  pm1=avb[i][j];
			  tma=j;
		  }
	  }

	  for(j=ptm;j<(ptm+tmaftsp);j++) {
		  if (avb[i][j]>pm2) {
			  pm2=avb[i][j];
			  tmb=j;
		  }
	  }
  }

  peak_to_valley_1_ = (ptv - pm1) / 2;
  peak_to_valley_2_ = (ptv - pm2) / 2;
  intervalley_ = (tma - tmb) * 200;
}

void Spike::set_peak_valley_features() {
	int peak_time, peak_value;

	// TODO parametrize
	find_one_peak(&peak_time, 64, 16, &peak_value);
	find_valleys(peak_time, peak_value, &peak_to_valley_1_, &peak_to_valley_2_, &intervalley_);
}

const float& Spike::getFeature(const unsigned int& index) const {
	if (index < num_channels_ * num_pc_){
		return pc[index];
	}
	else{
		return *(extra_features_[index - num_channels_ * num_pc_]);
	}
}

float* Spike::getFeatureAddr(const unsigned int& index) {
	if (index < num_channels_ * num_pc_){
		return pc + index;
	}
	else{
		return extra_features_[index - num_channels_ * num_pc_];
	}
}

void Spike::assignExtraFeaturePointers() {
	extra_features_[0] = &peak_to_valley_1_;
	extra_features_[1] = &peak_to_valley_2_;
	extra_features_[2] = &intervalley_;
	extra_features_[3] = &power_;
}

Spike::Spike() {
}
