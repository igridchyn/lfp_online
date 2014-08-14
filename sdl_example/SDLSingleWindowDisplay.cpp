//
//  SDLSingleWindowDisplay.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"

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
//, palette_(12, new int[12]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928}){
, palette_(ColorPalette::BrewerPalette12){
    
    window_ = SDL_CreateWindow(window_name.c_str(), 50,50,window_width, window_height, 0);
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
