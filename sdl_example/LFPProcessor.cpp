//
//  LFPProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 31/03/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include <fstream>

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


int TetrodesInfo::number_of_channels(Spike *spike){
    return channels_numbers[spike->tetrode_];
}

// ============================================================================================================

void LFPPipeline::process(unsigned char *data, int nchunks){
    // TODO: put data into buffer
    
    for (std::vector<LFPProcessor*>::const_iterator piter = processors.begin(); piter != processors.end(); ++piter) {
        (*piter)->process();
    }
}


// ============================================================================================================

SpikeDetectorProcessor::SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const float nstd, const int refractory)
    : LFPProcessor(buffer)
    ,last_processed_id(0)
    , nstd_(nstd)
    , refractory_(refractory)
{
    // load spike filter
    std::ifstream filter_stream;
    filter_stream.open(filter_path);
    
    int fpos = 0;
    while(!filter_stream.eof()){
         filter_stream >> filter[fpos++];
        
        // DEBUG
        // printf("filt: %f\n", filter[fpos-1]);
    }
    filter_len = fpos - 1;
    
    filt_pos = buffer->HEADER_LEN;
    
    buffer->last_spike_pos_ = - refractory_;
    //printf("filt len: %d\n", filter_len);
}

void SpikeDetectorProcessor::process()
{
    // for all channels
    // TODO: parallelize
    
    // printf("Spike detect...");
    
    // to start detection by threshold from this position after filtering + power computation
    int det_pos = filt_pos;
    
    for (int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {

        if (!buffer->is_valid_channel(channel))
            continue;

        for (int fpos = filt_pos; fpos < buffer->buf_pos - filter_len/2; ++fpos) {
            
            // filter with high-pass spike filter
            float filtered = 0;
            int *chan_sig_buf = buffer->signal_buf[channel] + fpos - filter_len/2;
            
            for (int j=0; j < filter_len; ++j, chan_sig_buf++) {
                filtered += *(chan_sig_buf) * filter[j];
            }
            
            buffer->filtered_signal_buf[channel][fpos] = filtered;
            
            // power in window of 4
            float pw = 0;
            for(int i=0;i<4;++i){
                pw += buffer->filtered_signal_buf[channel][fpos-i] * buffer->filtered_signal_buf[channel][fpos-i];
            }
            buffer->power_buf[channel][fpos] = sqrt(pw);

            // std estimation
            if (buffer->powerEstimatorsMap_[channel] != NULL)
                buffer->powerEstimatorsMap_[channel]->push((float)sqrt(pw));
            
            // DEBUG
//            if (channel == 8){
//                // printf("std estimate: %d\n", (int)buffer->powerEstimatorsMap_[channel]->get_std_estimate());
//            }
        }
    }
    
    for (int dpos = det_pos; dpos < buffer->buf_pos - filter_len/2; ++dpos) {
         for (int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {
             //float threshold = (int)buffer->powerEstimatorsMap_[channel]->get_std_estimate() * nstd_;
             // DEBUG
             int threshold = (int)(43.491423979974343 * nstd_);
             
             // detection via threshold nstd * std
             int spike_pos = buffer->last_pkg_id - buffer->buf_pos + dpos;
             if (buffer->power_buf[channel][dpos] > threshold && spike_pos - buffer->last_spike_pos_ > refractory_)
             {
                 // printf("Spike: %d...\n", spike_pos);
                 // printf("%d ", spike_pos);
                 
                 buffer->last_spike_pos_ = spike_pos + 1;
                 buffer->spike_buffer_[buffer->spike_buf_pos] = new Spike(spike_pos + 1, buffer->tetr_info_->tetrode_by_channel[channel]);
                 buffer->spike_buf_pos++;
                 
                 // check if rewind is requried
                 if (buffer->spike_buf_pos == buffer->SPIKE_BUF_HEAD_LEN - 1){
                     // TODO: !!! delete the rest of spikes !
                     memcpy(buffer->spike_buffer_ + buffer->SPIKE_BUF_HEAD_LEN, buffer->spike_buffer_ + buffer->SPIKE_BUF_HEAD_LEN - 1 - buffer->SPIKE_BUF_HEAD_LEN, sizeof(Spike*)*buffer->SPIKE_BUF_HEAD_LEN);
                     
                     buffer->spike_buf_no_rec = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_no_rec);
                     buffer->spike_buf_nows_pos = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_nows_pos);
                     buffer->spike_buf_pos_unproc_ = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_pos_unproc_);
                     buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_no_disp_pca);
                     buffer->spike_buf_pos_out = buffer->SPIKE_BUF_HEAD_LEN - (buffer->spike_buf_pos - buffer->spike_buf_pos_out);
                     
                     buffer->spike_buf_pos = buffer->SPIKE_BUF_HEAD_LEN;
                 }
             }
         }
    }
    
    filt_pos = buffer->buf_pos - filter_len / 2;
    
    //
    if (filt_pos < buffer->HEADER_LEN)
        filt_pos = buffer->HEADER_LEN;
}

// ============================================================================================================
void PackageExractorProcessor::process(){

    // see if buffer reinit is needed, rewind buffer
    if (buffer->buf_pos + 3 * buffer->num_chunks > buffer->LFP_BUF_LEN - buffer->BUF_HEAD_LEN){
        for (int c=0; c < buffer->CHANNEL_NUM; ++c){
            memcpy(buffer->signal_buf[c], buffer->signal_buf[c] + buffer->buf_pos - buffer->BUF_HEAD_LEN, buffer->BUF_HEAD_LEN * sizeof(int));
            memcpy(buffer->filtered_signal_buf[c], buffer->filtered_signal_buf[c] + buffer->buf_pos - buffer->BUF_HEAD_LEN, buffer->BUF_HEAD_LEN * sizeof(int));
        }

        buffer->zero_level = buffer->buf_pos + 1;
        buffer->buf_pos = buffer->BUF_HEAD_LEN;
    }
    else{
        buffer->zero_level = 0;
    }
    
    // data extraction:
    // t_bin *ch_dat =  (t_bin*)(block + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[CHANNEL]);
    
    unsigned char *bin_ptr = buffer->chunk_ptr + buffer->HEADER_LEN;
    for (int chunk=0; chunk < buffer->num_chunks; ++chunk, bin_ptr += buffer->TAIL_LEN) {
        for (int block=0; block < 3; ++block) {
            for (int c=0; c < buffer->CHANNEL_NUM; ++c, bin_ptr += 2) {
                // ??? is it necessary to order the channels?
                // ??? filter directly the data in bin buffer? [for better performance]
                
                // !!!??? +1 to make similar to *.dat
                buffer->signal_buf[buffer->CH_MAP_INV[c]][buffer->buf_pos + chunk*3 + block] = *((short*)bin_ptr) + 1;
                
                // DEBUG
                //if (c == 0)
                //    printf("%d\n", *((short*)bin_ptr) + 1);
            }
        }
    }
    
    buffer->buf_pos += 3 * buffer->num_chunks;
    buffer->last_pkg_id += 3 * buffer->num_chunks;
}

void SDLControlInputMetaProcessor::process(){
    // check meta-events, control change, pass control to current processor
 
    SDL_Event e;
    bool quit = false;
    while( SDL_PollEvent( &e ) != 0 )
    {
        //User requests quit
        if( e.type == SDL_QUIT )
        {
            quit = true;
        }
        else{
            control_processor_->process_SDL_control_input(e);
        }
    }
}

SDLControlInputMetaProcessor::SDLControlInputMetaProcessor(LFPBuffer* buffer, SDLControlInputProcessor* control_processor)
    : LFPProcessor(buffer)
    , control_processor_(control_processor)
{}

SDLPCADisplayProcessor::SDLPCADisplayProcessor(LFPBuffer *buf, SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture)
: LFPProcessor(buf)
, window_(window)
, renderer_(renderer)
, texture_(texture){
    
}

void SDLPCADisplayProcessor::process(){
    // TODO: parametrize displayed channels and pc numbers
    
    const int rend_freq = 5;
    bool render = false;
    
    while (buffer->spike_buf_no_disp_pca < buffer->spike_buf_pos_unproc_) {
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_no_disp_pca];
        if (spike->pc == NULL)
        {
            if (spike->discarded_){
                buffer->spike_buf_no_disp_pca++;
                continue;
            }
            else{
                break;
            }
        }

        int x = spike->pc[0][0] + 600;
        int y = spike->pc[0][1] + 300;
        
        SDL_SetRenderTarget(renderer_, texture_);
        SDL_SetRenderDrawColor(renderer_, 255,255,255,255);
        SDL_RenderDrawPoint(renderer_, x, y);
        
        buffer->spike_buf_no_disp_pca++;
        
        if (!(buffer->spike_buf_no_disp_pca % rend_freq))
            render = true;
    }
    
    if (render){
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);
    }
}

FileOutputProcessor::FileOutputProcessor(LFPBuffer* buf)
    : LFPProcessor(buf){
        f_ = fopen("/Users/igridchyn/Dropbox/IST_Austria/Csicsvari/Data Processing/spike_detection/cpp/cppout.txt", "w");
}

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

// ============================================================================================================

LFPBuffer::LFPBuffer(TetrodesInfo* tetr_info)
    :tetr_info_(tetr_info)
{
    for(int c=0; c < CHANNEL_NUM; ++c){
        memset(signal_buf[c], LFP_BUF_LEN, sizeof(int));
        memset(filtered_signal_buf[c], LFP_BUF_LEN, sizeof(int));
        memset(power_buf[c], LFP_BUF_LEN, sizeof(int));
        powerEstimatorsMap_[c] = NULL;
    }
    
    powerEstimators_ = new OnlineEstimator<float>[tetr_info_->tetrodes_number];

    tetr_info_->tetrode_by_channel = new int[CHANNEL_NUM];
    
    // create a map of pointers to tetrode power estimators for each electrode
    for (int tetr = 0; tetr < tetr_info_->tetrodes_number; ++tetr ){
        for (int ci = 0; ci < tetr_info_->channels_numbers[tetr]; ++ci){
            powerEstimatorsMap_[tetr_info_->tetrode_channels[tetr][ci]] = powerEstimators_ + tetr;
            is_valid_channel_[tetr_info_->tetrode_channels[tetr][ci]] = true;
            
            tetr_info_->tetrode_by_channel[tetr_info_->tetrode_channels[tetr][ci]] = tetr;
        }
    }
    
}

inline bool LFPBuffer::is_valid_channel(int channel_num){
    return is_valid_channel_[channel_num];
}

// ============================================================================================================

template<class T>
T OnlineEstimator<T>::get_mean_estimate(){
    return sumsq / num_samples;
}

template<class T>
T OnlineEstimator<T>::get_std_estimate(){
    // printf("sumsq: %f, num samp: %d\n", sumsq, num_samples);
    return (T)sqrt(sumsq / num_samples - (sum / num_samples) * (sum / num_samples));
}

template<class T>
void OnlineEstimator<T>::push(T value){
    // printf("push %f\n", value);
    
    // TODO: ignore to optimize ?
    if (num_samples < BUF_SIZE){
        num_samples ++;
    }
    else{
        sum -= buf[buf_pos];
        sumsq -= buf[buf_pos] * buf[buf_pos];
    }
        
    // update estimates
    buf[buf_pos] = value;
    sum += buf[buf_pos];
    sumsq += buf[buf_pos] * buf[buf_pos];
    
    buf_pos = (buf_pos + 1) % BUF_SIZE;
}