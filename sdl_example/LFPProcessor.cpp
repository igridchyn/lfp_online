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

        std::cout << "SIGNAL BUFFER REWIND (at pos " << buffer->buf_pos <<  ")!\n";
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
        
        buffer->positions_buf_[buffer->pos_buf_pos_][0] = (unsigned int)bx;
        buffer->positions_buf_[buffer->pos_buf_pos_][1] = (unsigned int)by;
        buffer->positions_buf_[buffer->pos_buf_pos_][2] = (unsigned int)sx;
        buffer->positions_buf_[buffer->pos_buf_pos_][3] = (unsigned int)sy;
        buffer->positions_buf_[buffer->pos_buf_pos_][4] = (unsigned int)buffer->last_pkg_id;
        
        // speed estimation
        // TODO: use average of bx, sx or alike
        // TODO: deal with missing points
        if (buffer->pos_buf_pos_ > 16 && bx != 1023 && buffer->positions_buf_[buffer->pos_buf_pos_ - 16][0] != 1023){
            float dx = (float)bx - buffer->positions_buf_[buffer->pos_buf_pos_ - 16][0];
            float dy = (float)by - buffer->positions_buf_[buffer->pos_buf_pos_ - 16][1];
            buffer->speedEstimator_->push(sqrt(dx * dx + dy * dy));
            // TODO: float / scale ?
            buffer->positions_buf_[buffer->pos_buf_pos_ - 8][5] = buffer->speedEstimator_->get_mean_estimate();
//            std::cout << "speed= " << buffer->speedEstimator_->get_mean_estimate() << "\n";
            
            // update spike speed
            int known_speed_pkg_id =  buffer->positions_buf_[buffer->pos_buf_pos_ - 8][4];
            while (true){
                Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_speed_];
                if (spike == NULL || spike->pkg_id_ > known_speed_pkg_id){
                    break;
                }
                
                // find last position sample before the spike
                while(buffer->pos_buf_pos_spike_speed_ < buffer->pos_buf_pos_ - 8 && buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_ + 1][4] < spike->pkg_id_){
                    buffer->pos_buf_pos_spike_speed_ ++;
                }
                
                // interpolate speed during spike :
                int diff_bef = spike->pkg_id_ - buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_][4];
                int diff_aft = (int)(known_speed_pkg_id - spike->pkg_id_);
                float w_bef = 1/(float)(diff_bef + 1);
                float w_aft = 1/(float)(diff_aft + 1);
                // TODO: weights ?
                spike->speed = ( buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_][5] * w_bef + buffer->positions_buf_[buffer->pos_buf_pos_ - 8][5] * w_aft) / (float)(w_bef + w_aft);
//                std::cout << "Spike speed " << spike->speed << "\n";
                
                buffer->spike_buf_pos_speed_ ++;
            }
        }
        
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
                            // TODO: out of range check
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

// !!! TODO: INTRODUCE PALETTE WITH LARGER NUMBER OF COLOURS (but categorical)
const ColorPalette ColorPalette::BrewerPalette12(20, new int[20]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928, 0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00});

const ColorPalette ColorPalette::MatlabJet256(256, new int[256] {0x83, 0x87, 0x8b, 0x8f, 0x93, 0x97, 0x9b, 0x9f, 0xa3, 0xa7, 0xab, 0xaf, 0xb3, 0xb7, 0xbb, 0xbf, 0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7, 0xdb, 0xdf, 0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff, 0x3ff, 0x7ff, 0xbff, 0xfff, 0x13ff, 0x17ff, 0x1bff, 0x1fff, 0x23ff, 0x27ff, 0x2bff, 0x2fff, 0x33ff, 0x37ff, 0x3bff, 0x3fff, 0x43ff, 0x47ff, 0x4bff, 0x4fff, 0x53ff, 0x57ff, 0x5bff, 0x5fff, 0x63ff, 0x67ff, 0x6bff, 0x6fff, 0x73ff, 0x77ff, 0x7bff, 0x7fff, 0x83ff, 0x87ff, 0x8bff, 0x8fff, 0x93ff, 0x97ff, 0x9bff, 0x9fff, 0xa3ff, 0xa7ff, 0xabff, 0xafff, 0xb3ff, 0xb7ff, 0xbbff, 0xbfff, 0xc3ff, 0xc7ff, 0xcbff, 0xcfff, 0xd3ff, 0xd7ff, 0xdbff, 0xdfff, 0xe3ff, 0xe7ff, 0xebff, 0xefff, 0xf3ff, 0xf7ff, 0xfbff, 0xffff, 0x3fffb, 0x7fff7, 0xbfff3, 0xfffef, 0x13ffeb, 0x17ffe7, 0x1bffe3, 0x1fffdf, 0x23ffdb, 0x27ffd7, 0x2bffd3, 0x2fffcf, 0x33ffcb, 0x37ffc7, 0x3bffc3, 0x3fffbf, 0x43ffbb, 0x47ffb7, 0x4bffb3, 0x4fffaf, 0x53ffab, 0x57ffa7, 0x5bffa3, 0x5fff9f, 0x63ff9b, 0x67ff97, 0x6bff93, 0x6fff8f, 0x73ff8b, 0x77ff87, 0x7bff83, 0x7fff7f, 0x83ff7b, 0x87ff77, 0x8bff73, 0x8fff6f, 0x93ff6b, 0x97ff67, 0x9bff63, 0x9fff5f, 0xa3ff5b, 0xa7ff57, 0xabff53, 0xafff4f, 0xb3ff4b, 0xb7ff47, 0xbbff43, 0xbfff3f, 0xc3ff3b, 0xc7ff37, 0xcbff33, 0xcfff2f, 0xd3ff2b, 0xd7ff27, 0xdbff23, 0xdfff1f, 0xe3ff1b, 0xe7ff17, 0xebff13, 0xefff0f, 0xf3ff0b, 0xf7ff07, 0xfbff03, 0xffff00, 0xfffb00, 0xfff700, 0xfff300, 0xffef00, 0xffeb00, 0xffe700, 0xffe300, 0xffdf00, 0xffdb00, 0xffd700, 0xffd300, 0xffcf00, 0xffcb00, 0xffc700, 0xffc300, 0xffbf00, 0xffbb00, 0xffb700, 0xffb300, 0xffaf00, 0xffab00, 0xffa700, 0xffa300, 0xff9f00, 0xff9b00, 0xff9700, 0xff9300, 0xff8f00, 0xff8b00, 0xff8700, 0xff8300, 0xff7f00, 0xff7b00, 0xff7700, 0xff7300, 0xff6f00, 0xff6b00, 0xff6700, 0xff6300, 0xff5f00, 0xff5b00, 0xff5700, 0xff5300, 0xff4f00, 0xff4b00, 0xff4700, 0xff4300, 0xff3f00, 0xff3b00, 0xff3700, 0xff3300, 0xff2f00, 0xff2b00, 0xff2700, 0xff2300, 0xff1f00, 0xff1b00, 0xff1700, 0xff1300, 0xff0f00, 0xff0b00, 0xff0700, 0xff0300, 0xff0000, 0xfb0000, 0xf70000, 0xf30000, 0xef0000, 0xeb0000, 0xe70000, 0xe30000, 0xdf0000, 0xdb0000, 0xd70000, 0xd30000, 0xcf0000, 0xcb0000, 0xc70000, 0xc30000, 0xbf0000, 0xbb0000, 0xb70000, 0xb30000, 0xaf0000, 0xab0000, 0xa70000, 0xa30000, 0x9f0000, 0x9b0000, 0x970000, 0x930000, 0x8f0000, 0x8b0000, 0x870000, 0x830000, 0x7f0000});

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
}

// ============================================================================================================

void SDLSingleWindowDisplay::FillRect(const int x, const int y, const int cluster, const unsigned int w, const unsigned int h){
    SDL_Rect rect;
    rect.h = h;
    rect.w = w;
    rect.x = x-w/2;
    rect.y = y-h/2;

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

const char * const Utils::NUMBERS[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61", "62", "63"};