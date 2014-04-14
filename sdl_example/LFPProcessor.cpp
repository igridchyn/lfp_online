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

SpikeDetectorProcessor::SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const float detection_threshold)
    : LFPProcessor(buffer)
    ,last_processed_id(0)
    , detection_threshold(detection_threshold)
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
    
    //printf("filt len: %d\n", filter_len);
}

void SpikeDetectorProcessor::process()
{
    // for all channels
    // TODO: parallelize
    for (int channel=0; channel<buffer->CHANNEL_NUM; ++channel) {

        for (int fpos = filt_pos; fpos < buffer->buf_pos - buffer->HEADER_LEN - filter_len / 2; ++fpos) {
            
            // filter with high-pass spike filter
            float filtered = 0;
            int *chan_sig_buf = buffer->signal_buf[channel] + buffer->HEADER_LEN + fpos - filter_len / 2;
            
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
            
            // DEBUG
            if (channel == 8)
                printf("%d\n", (int)sqrt(pw));
        }
    }
    
    filt_pos = buffer->buf_pos - buffer->HEADER_LEN - filter_len / 2;
}

// ============================================================================================================
void PackageExractorProcessor::process(){

    // see if buffer reinit is needed
    if (buffer->buf_pos + 3 * buffer->num_chunks > buffer->LFP_BUF_LEN - buffer->BUF_HEAD_LEN){
        for (int c=0; c < buffer->CHANNEL_NUM; ++c){
            memccpy(buffer->signal_buf[c], buffer->signal_buf[c] + buffer->buf_pos, buffer->BUF_HEAD_LEN, sizeof(unsigned char));
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

// ============================================================================================================

Spike::Spike(int *buffer, int pkg_id, int channel)
    : pkg_id(pkg_id)
{
    // pos is where spike has been detected -> so, approx. middle of the spike
    for (int s = -WL_LENGTH/2; s < WL_LENGTH/2; ++s){
        // BS
        waveshape[s + WL_LENGTH/2] = buffer[pkg_id];
    }
}

inline int LFPBuffer::get_signal(int channel, int pkg_id){
    return signal_buf[channel][(pkg_id - last_pkg_id + buf_pos) % LFP_BUF_LEN];
}