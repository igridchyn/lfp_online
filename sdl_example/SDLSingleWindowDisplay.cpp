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

void SDLSingleWindowDisplay::ResetTextStack() {
	text_stack_height_ = 0;
}

SDLSingleWindowDisplay::~SDLSingleWindowDisplay(){
	SDL_DestroyWindow(window_);
}

SDLSingleWindowDisplay::SDLSingleWindowDisplay(std::string window_name, const unsigned int& window_width, const unsigned int& window_height)
: window_width_(window_width)
, window_height_(window_height)
, name_(window_name)
//, palette_(12, new int[12]{0xA6CEE3, 0x1F78B4, 0xB2DF8A, 0x33A02C, 0xFB9A99, 0xE31A1C, 0xFDBF6F, 0xFF7F00, 0xCAB2D6, 0x6A3D9A, 0xFFFF99, 0xB15928}){
, palette_(ColorPalette::BrewerPalette12){
    
    window_ = SDL_CreateWindow(window_name.c_str(), 50,50,window_width, window_height, 0);
#ifdef _WIN32
	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
#else
	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
#endif

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


unsigned int SDLSingleWindowDisplay::GetWindowID() {
	return SDL_GetWindowID(window_);
}


void SDLSingleWindowDisplay::TextOut(std::string text, int x, int y) {
	TTF_Init();
	TTF_Font *font = TTF_OpenFont("FreeSerif.ttf", 15);
	SDL_Color color = { 255, 255, 255 };
	SDL_Surface * surface = TTF_RenderText_Solid(font, text.c_str(), color);
	SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer_,  surface);
	int texW = 0;
	int texH = 0;
	SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
	SDL_Rect dstrect = { x, y, texW, texH };
	SDL_RenderCopy(renderer_, texture, nullptr, &dstrect);
	TTF_CloseFont(font);
	TTF_Quit();

	text_stack_height_ += texH;
}

void SDLSingleWindowDisplay::TextOut(std::string text) {
	TextOut(text, 0, text_stack_height_);
}
