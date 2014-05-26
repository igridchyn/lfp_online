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

LFPProcessor *LFPPipeline::get_processor(const unsigned int& index){
    return processors[index];
}

std::vector<SDLControlInputProcessor *> LFPPipeline::GetSDLControlInputProcessors(){
    // TODO: use vector
    std::vector<SDLControlInputProcessor *> control_processors;
    
    for (int p=0; p<processors.size(); ++p) {
        SDLControlInputProcessor *ciproc = dynamic_cast<SDLControlInputProcessor*>(processors[p]);
        if (ciproc != NULL){
            control_processors.push_back(ciproc);
        }
    }
    
    return control_processors;
}

// ============================================================================================================

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
        buffer->positions_buf_[buffer->pos_buf_pos_][4] = buffer->last_pkg_id;
        
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
            // check for control switch
            if( e.type == SDL_KEYDOWN ){
                SDL_Keymod kmod = SDL_GetModState();
                if (kmod & KMOD_LCTRL){
                    // switch to corresponding processor
                    switch( e.key.keysym.sym )
                    {
                            // TODO: out of range checl
                            case SDLK_1:
                                control_processor_ = control_processors_[0];
                                break;
                            case SDLK_2:
                                control_processor_ = control_processors_[1];
                                break;
                            case SDLK_3:
                                control_processor_ = control_processors_[2];
                                break;
                            case SDLK_4:
                                control_processor_ = control_processors_[3];
                                break;
                            case SDLK_5:
                                control_processor_ = control_processors_[4];
                                break;
                    }
                    
                    continue;
                }
                
                if (kmod & KMOD_LALT){
                    // switch tetrode
                    
                    switch( e.key.keysym.sym )
                    {
                        // TODO: all tetrodes (10-16: numpad; 17-32: RALT)
                            
                        case SDLK_1:
                            SwitchDisplayTetrode(0);
                            break;
                        case SDLK_2:
                            SwitchDisplayTetrode(1);
                            break;
                        case SDLK_3:
                            SwitchDisplayTetrode(2);
                            break;
                        case SDLK_4:
                            SwitchDisplayTetrode(3);
                            break;
                        case SDLK_5:
                            SwitchDisplayTetrode(4);
                            break;
                    }
                    
                    continue;
                }
            }
            
            control_processor_->process_SDL_control_input(e);
        }
    }
}

SDLControlInputMetaProcessor::SDLControlInputMetaProcessor(LFPBuffer* buffer, std::vector<SDLControlInputProcessor *> control_processors)
    : LFPProcessor(buffer)
    , control_processor_(control_processors[0])
    , control_processors_(control_processors)
{}

void SDLControlInputMetaProcessor::SwitchDisplayTetrode(const unsigned int& display_tetrode){
    for (int pi=0; pi < control_processors_.size(); ++pi) {
        control_processors_[pi]->SetDisplayTetrode(display_tetrode);
    }
}

SDLControlInputProcessor::SDLControlInputProcessor(LFPBuffer *buf)
: LFPProcessor(buf) { }

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

const ColorPalette ColorPalette::BrewerPalette12(12, new int[12]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928});

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
    
    last_spike_pos_ = new int[tetr_info_->tetrodes_number];
}

// ============================================================================================================

void SDLSingleWindowDisplay::FillRect(const int x, const int y, const int cluster){
    const int sz = 4;
    
    SDL_Rect rect;
    rect.h = sz;
    rect.w = sz;
    rect.x = x-sz/2;
    rect.y = y-sz/2;

    SDL_SetRenderDrawColor(renderer_, palette_.getR(cluster), palette_.getG(cluster), palette_.getB(cluster), 255);
    SDL_RenderFillRect(renderer_, &rect);
}


SDLSingleWindowDisplay::SDLSingleWindowDisplay(std::string window_name, const unsigned int& window_width, const unsigned int& window_height)
    : window_width_(window_width)
    , window_height_(window_height)
    , palette_(12, new int[12]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928}){
    
    window_ = SDL_CreateWindow(window_name.c_str(), 0,0,window_width, window_height, 0);
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, window_height);
    //SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND); // ???
    
    SDL_SetRenderTarget(renderer_, texture_);
    
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
}


void SDLSingleWindowDisplay::ReinitScreen(){
    SDL_SetRenderTarget(renderer_, texture_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
}

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

FetReaderProcessor::FetReaderProcessor(LFPBuffer *buf, std::string fet_path)
: LFPProcessor(buf) {
    int ncomp;
    fet_file_ = std::ifstream(fet_path);
    fet_file_ >> ncomp;
}

void FetReaderProcessor::process(){
    if (fet_file_.eof())
        return;
    
    Spike *spike = new Spike(0, 0);
    const int ntetr = 4;
    const int npc = 3;
    
    spike->pc = new float*[ntetr];
    
    for (int t=0; t < ntetr; ++t) {
        spike->pc[t] = new float[npc];
        for (int pc=0; pc < npc; ++pc) {
            fet_file_ >> spike->pc[t][pc];
            spike->pc[t][pc] /= 5;
        }
    }
    
    int dummy;
    for (int d=0; d < 4; ++d) {
        fet_file_ >> dummy;
    }
    
    int stime;
    fet_file_ >> stime;
    spike->pkg_id_ = stime;
    spike->aligned_ = true;
    
    // TODO: add_spike() + buffer rewind
    buffer->spike_buffer_[buffer->spike_buf_pos++] = spike;
    // for clustering
    buffer->spike_buf_pos_unproc_++;
}