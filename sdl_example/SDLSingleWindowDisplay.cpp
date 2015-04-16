//
//  SDLSingleWindowDisplay.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "SDLSingleWindowDisplay.h"

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
    SDL_RenderPresent(renderer_);
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


void SDLSingleWindowDisplay::TextOut(std::string text, int x, int y, int col, bool shift) {
	TTF_Init();
#ifndef _WIN32
	TTF_Font *font = TTF_OpenFont("FreeSerif.ttf", 15);
#else
	TTF_Font *font = TTF_OpenFont("../Res/FORTE.TTF", 15);
#endif
	SDL_Color color = { ColorPalette::getColorR(col), ColorPalette::getColorG(col), ColorPalette::getColorB(col) };
	SDL_Surface * surface = TTF_RenderText_Solid(font, text.c_str(), color);
	SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer_,  surface);
	int texW = 0;
	int texH = 0;
	SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
	SDL_Rect dstrect = { x, y, texW, texH };
	SDL_RenderCopy(renderer_, texture, nullptr, &dstrect);
	TTF_CloseFont(font);
	TTF_Quit();

	text_stack_width_ += texW;

	if (shift){
		text_stack_height_ += texH;
		text_stack_width_ = 0;
	}
}

void SDLSingleWindowDisplay::TextOut(std::string text, int col, bool shift) {
	TextOut(text, text_stack_width_, text_stack_height_, col, shift);
}

void SDLSingleWindowDisplay::DrawCross(int w, int x, int y) {
	int cw = w;
	SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 0);
	SDL_RenderDrawLine(renderer_, x-cw, y-cw, x+cw, y+cw);
	SDL_RenderDrawLine(renderer_, x-cw, y+cw, x+cw, y-cw);
}

void SDLSingleWindowDisplay::DrawCross(int w, int x, int y, int coli) {
	int cw = w;
	SDL_SetRenderDrawColor(renderer_, palette_.getR(coli), palette_.getG(coli), palette_.getB(coli), 0);
//	SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 0);
	SDL_RenderDrawLine(renderer_, x-cw, y-cw, x+cw, y+cw);
	SDL_RenderDrawLine(renderer_, x-cw, y+cw, x+cw, y-cw);
}

void SDLSingleWindowDisplay::DrawRect(const int& x, const int& y, const int& w,
		const int& h, const int& col_id) {
	 SDL_Rect rect;
	 rect.h = h;
	 rect.w = w;
	 rect.x = x;
	 rect.y = y;

	 SDL_SetRenderDrawColor(renderer_, palette_.getR(col_id), palette_.getG(col_id), palette_.getB(col_id), 255);
	 SDL_RenderDrawRect(renderer_, &rect);
}

void SDLSingleWindowDisplay::SetDrawColor(int cid) {
	SDL_SetRenderDrawColor(renderer_, palette_.getR(cid), palette_.getG(cid), palette_.getB(cid), 255);
}
