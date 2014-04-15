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
            if (channel == 8){
                // printf("std estimate: %d\n", (int)buffer->powerEstimatorsMap_[channel]->get_std_estimate());
            }

            //float threshold = (int)buffer->powerEstimatorsMap_[channel]->get_std_estimate() * nstd_;
            // DEBUG
            float threshold = 43.491423979974343 * nstd_;
            
            // detection via threshold nstd * std
            int spike_pos = buffer->last_pkg_id - buffer->buf_pos + fpos;
            if (buffer->power_buf[channel][fpos] > threshold && spike_pos - buffer->last_spike_pos_ > refractory_)
            {
                printf("Spike: %d...\n", spike_pos);
                buffer->last_spike_pos_ = spike_pos + 1;
                buffer->spike_buffer_[buffer->spike_buf_pos] = new Spike(spike_pos + 1, buffer->tetr_info_->tetrode_by_channel[channel]);
                buffer->spike_buf_pos++;
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

    // see if buffer reinit is needed
    if (buffer->buf_pos + 3 * buffer->num_chunks > buffer->LFP_BUF_LEN - buffer->BUF_HEAD_LEN){
        for (int c=0; c < buffer->CHANNEL_NUM; ++c){
            memcpy(buffer->signal_buf[c], buffer->signal_buf[c] + buffer->buf_pos, buffer->BUF_HEAD_LEN * sizeof(unsigned char));
            memcpy(buffer->filtered_signal_buf[c], buffer->filtered_signal_buf[c] + buffer->buf_pos, buffer->BUF_HEAD_LEN * sizeof(unsigned char));
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

void SDLSignalDisplayProcessor::process(){
    // buffer has been reinitizlied
    if ( buffer->zero_level > 0 )
    {
        last_disp_pos = buffer->BUF_HEAD_LEN - (buffer->BUF_HEAD_LEN - buffer->zero_level) % plot_hor_scale;
    }
    
    // whether to display
    for (int pos = last_disp_pos + plot_hor_scale; pos < buffer->buf_pos; pos += plot_hor_scale){
        int val = transform_to_y_coord(buffer->signal_buf[target_channel_][pos]);
        
        SDL_SetRenderTarget(renderer_, texture_);
        drawLine(renderer_, current_x, val_prev, current_x + 1, val);
        
        current_x++;
        
        if (current_x == SCREEN_WIDTH - 1){
            current_x = 1;
            
            // reset screen
            SDL_SetRenderTarget(renderer_, texture_);
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
            SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
            SDL_RenderPresent(renderer_);
        }
        
        last_disp_pos = pos;
        val_prev = val;
    }
    
    // whether to render
    if (unrendered > DISP_FREQ * plot_hor_scale){
        SDL_SetRenderTarget(renderer_, NULL);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);
        
        unrendered = 0;
    }
    else{
        unrendered += buffer->buf_pos - last_disp_pos;
    }
    
    return;
    
    if (!(buffer->buf_pos % plot_hor_scale)){
        
        // whether to render
        if (!(buffer->buf_pos % DISP_FREQ * plot_hor_scale)){
            SDL_SetRenderTarget(renderer_, NULL);
            SDL_RenderCopy(renderer_, texture_, NULL, NULL);
            SDL_RenderPresent(renderer_);
        }
        
        if (current_x == SCREEN_WIDTH - 1){
            current_x = 1;
            
            // reset screen
            SDL_SetRenderTarget(renderer_, texture_);
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
            SDL_RenderDrawLine(renderer_, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
            SDL_RenderPresent(renderer_);
        }
    }
    
    
}

int SDLSignalDisplayProcessor::transform_to_y_coord(int voltage){
    // scale for plotting
    int val = voltage + SHIFT;
    val = val > 0 ? val / plot_scale : 1;
    val = val < SCREEN_HEIGHT ? val : SCREEN_HEIGHT;
    
    return val;
}

void SDLSignalDisplayProcessor::drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2){
    SDL_SetRenderDrawColor(renderer_, 255,255,255,255);
    SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
}

void SDLSignalDisplayProcessor::process_SDL_control_input(const SDL_Event &e){
    //User requests quit
    if( e.type == SDL_KEYDOWN )
    {
        //Select surfaces based on key press
        switch( e.key.keysym.sym )
        {
            case SDLK_UP:
                plot_scale += 5;
                break;
            case SDLK_DOWN:
                plot_scale = plot_scale > 5  ? plot_scale - 5 : 5;
                break;
                
            case SDLK_RIGHT:
                plot_hor_scale += 2;
                break;
            case SDLK_LEFT:
                plot_hor_scale = plot_hor_scale > 2  ? plot_hor_scale - 2 : 2;
                break;
                
            case SDLK_ESCAPE:
                exit(0);
                break;
            case SDLK_1:
                target_channel_ = 1;
                break;
            case SDLK_2:
                target_channel_ = 5;
                break;
            case SDLK_3:
                target_channel_ = 9;
                break;
            case SDLK_4:
                target_channel_ = 13;
                break;
            case SDLK_5:
                target_channel_ = 17;
                break;
        }
    }
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

void SpikeAlignmentProcessor::process(){
    // try to populate unpopulated spikes
    while (buffer->spike_buf_nows_pos < buffer->spike_buf_pos && buffer->spike_buffer_[buffer->spike_buf_nows_pos]->pkg_id_ < buffer->last_pkg_id - 16){
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_nows_pos];
        int num_of_ch = buffer->tetr_info_->channels_numbers[spike->tetrode_];
        spike->waveshape = new int*[num_of_ch];
        
        for (int ch=0; ch < num_of_ch; ++ch){
            spike->waveshape[ch] = new int[Spike::WL_LENGTH];
            
            // copy signal for the channel from buffer to spike object
            memcpy(spike->waveshape[ch], buffer->signal_buf[buffer->tetr_info_->tetrode_channels[spike->tetrode_][ch]] + buffer->buf_pos - (buffer->last_pkg_id - spike->pkg_id_) - Spike::WL_LENGTH / 2, Spike::WL_LENGTH * sizeof(int));
            
            // DEBUG
            printf("Waveshape, %d, ch. %d: ", spike->pkg_id_, ch);
            for (int i=0; i<spike->WL_LENGTH;++i){
                printf("%d ", spike->waveshape[ch][i]);
            }
            printf("\n");
        }
        
        buffer-> spike_buf_nows_pos++;
    }
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