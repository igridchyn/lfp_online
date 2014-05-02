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
        
        filter_int_[fpos - 1] = 100000000 * filter[fpos - 1];
        
        // DEBUG
        // printf("filt: %f\n", filter[fpos-1]);
    }
    filter_len = fpos - 1;
    
    filt_pos = buffer->HEADER_LEN;
    
    buffer->last_spike_pos_ = - refractory_;
    //printf("filt len: %d\n", filter_len);
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
    
    char pos_flag = *((char*)buffer->chunk_ptr + 3);
    if (pos_flag == '2'){
        // extract position
        unsigned short bx = *((unsigned short*)(buffer->chunk_ptr + 16));
        unsigned short by = *((unsigned short*)(buffer->chunk_ptr + 18));
        unsigned short sx = *((unsigned short*)(buffer->chunk_ptr + 20));
        unsigned short sy = *((unsigned short*)(buffer->chunk_ptr + 22));
        
        buffer->positions_buf_[buffer->pos_buf_pos_][0] = bx;
        buffer->positions_buf_[buffer->pos_buf_pos_][1] = by;
        buffer->positions_buf_[buffer->pos_buf_pos_][2] = sx;
        buffer->positions_buf_[buffer->pos_buf_pos_][3] = sy;
        
        buffer->pos_buf_pos_++;
    }
    
    for (int chunk=0; chunk < buffer->num_chunks; ++chunk, bin_ptr += buffer->TAIL_LEN) {
        for (int block=0; block < 3; ++block) {
            short * sbin_ptr = (short*)bin_ptr;
            for (int c=0; c < buffer->CHANNEL_NUM; ++c, sbin_ptr++) {
                // ??? is it necessary to order the channels?
                // ??? filter directly the data in bin buffer? [for better performance]
                
                // !!!??? +1 to make similar to *.dat
                buffer->signal_buf[buffer->CH_MAP_INV[c]][buffer->buf_pos + chunk*3 + block] = *(sbin_ptr) + 1;
                
                // DEBUG
                //if (c == 0)
                //    printf("%d\n", *((short*)bin_ptr) + 1);
            }
            bin_ptr += 2 *  buffer->CHANNEL_NUM;
        }
    }
    
    buffer->buf_pos += 3 * buffer->num_chunks;
    buffer->last_pkg_id += 3 * buffer->num_chunks;
}

void SDLControlInputMetaProcessor::process(){
    // check meta-events, control change, pass control to current processor
 
    // for effectiveness: perform analysis every input_scan_rate_ packages
    // TODO: select reasonable rate
    if (buffer->last_pkg_id - last_input_pkg_id_ < input_scan_rate_)
        return;
    else
        last_input_pkg_id_ = buffer->last_pkg_id;
    
    SDL_Event e;
    bool quit = false;
    
    // SDL_PollEvent took 2/3 of runtime without limitations
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

SDLPCADisplayProcessor::SDLPCADisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int window_width, const unsigned int window_height)
: LFPProcessor(buffer)
, SDLSingleWindowDisplay(window_name, window_width, window_height)
{
    
}

void SDLPCADisplayProcessor::process(){
    // TODO: parametrize displayed channels and pc numbers
    
    const int rend_freq = 5;
    bool render = false;
    
    while (buffer->spike_buf_no_disp_pca < buffer->spike_buf_pos_unproc_) {
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_no_disp_pca];
        // wait until cluster is assigned
        if (spike->pc == NULL || spike->cluster_id_ == -1)
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
        SDL_SetRenderDrawColor(renderer_, 255,255,255*((int)spike->cluster_id_/2),255);
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

SDLSingleWindowDisplay::SDLSingleWindowDisplay(std::string window_name, const unsigned int& window_width, const unsigned int& window_height)
    : window_width_(window_width)
    , window_height_(window_height) {
    
    window_ = SDL_CreateWindow(window_name.c_str(), 0,0,window_width, window_height, 0);
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, window_height);
    //SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND); // ???
    
    SDL_SetRenderTarget(renderer_, texture_);
    
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
}