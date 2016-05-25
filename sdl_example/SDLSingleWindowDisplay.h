#ifndef SDLSINGLEWINDOWDISPLAY_H_
#define SDLSINGLEWINDOWDISPLAY_H_

#include <SDL2/SDL.h>

#include <string>
#include "LFPProcessor.h"

class SDLSingleWindowDisplay{
	friend class SDLControlInputMetaProcessor;

protected:
    SDL_Window *window_ = nullptr;
	SDL_Renderer *renderer_ = nullptr;
	SDL_Texture *texture_ = nullptr;

    const unsigned int window_width_;
    const unsigned int window_height_;

    std::string name_;

    ColorPalette palette_;

    unsigned int text_stack_width_ = 0;
	unsigned int text_stack_height_ = 0;

	unsigned int last_text_width_ = 0;

    virtual void FillRect(const int x, const int y, const int cluster, const unsigned int w = 4, const unsigned int h = 4);
    virtual void DrawRect(const int& x, const int& y, const int& w, const int& h, const int& col_id);

    // TEXT
    virtual void ResetTextStack();
    virtual void TextOut(std::string text, int x, int y, int color = 0xFFFFFF, bool shift = true);
    virtual void TextOut(std::string text, int col = 0xFFFFFF, bool shift = true);

    // Graphics
    virtual void DrawCross(int w, int x, int y);
    virtual void DrawCross(int w, int x, int y, int coli);
    virtual void SetDrawColor(int cluster);

    virtual void Render();
    virtual void RenderClear();

public:
    SDLSingleWindowDisplay(std::string window_name, const unsigned int& window_width, const unsigned int& window_height);
    virtual unsigned int GetWindowID();

	virtual ~SDLSingleWindowDisplay();
};

#endif // SDLSINGLEWINDOWDISPLAY_H_

