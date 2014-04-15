#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>
#include "LFPProcessor.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 600;

void putPixel(SDL_Renderer *renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    SDL_RenderDrawPoint(renderer, x, y);
}

void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2){
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

typedef short t_bin;

void draw_bin(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture, const char *path){
    FILE *f = fopen(path, "rb");
    
    const int CHUNK_SIZE = 432; // bytes
    
    unsigned char block[ CHUNK_SIZE ];
    int plot_scale = 40;
    const int SHIFT = 3000;
    
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);

    TetrodesInfo *tetr_inf = new TetrodesInfo();
    tetr_inf->tetrodes_number = 1;
    tetr_inf->channels_numbers = new int[1]{4};
    tetr_inf->tetrode_channels = new int*[1]{new int[4]{8,9,10,11}};
    
    LFPBuffer *buf = new LFPBuffer(tetr_inf);

    LFPPipeline *pipeline = new LFPPipeline();
    SDLSignalDisplayProcessor *sdlSigDispProc = new SDLSignalDisplayProcessor(buf, window, renderer, texture, 0);
    
    const char* filt_path = "/Users/igridchyn/Dropbox/IST_Austria/Csicsvari/Data Processing/spike_detection//filters/24k800-8000-50.txt";
    pipeline->add_processor(new PackageExractorProcessor(buf));
    pipeline->add_processor(new SpikeDetectorProcessor(buf, filt_path, 5.0, 16));
    pipeline->add_processor(new SpikeAlignmentProcessor(buf));
    pipeline->add_processor(sdlSigDispProc);
    pipeline->add_processor(new SDLControlInputMetaProcessor(buf, sdlSigDispProc));
    
    for (int i = 0; i < 1000000; ++i){
        fread((void*)block, CHUNK_SIZE, 1, f);
        
        buf->chunk_ptr = block;
        buf->num_chunks = 1;
        
        pipeline->process(block, 1);
        continue;

    }
}

void draw_test(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture){
    for(int x=0;x<10;x++)
    {
        SDL_SetRenderTarget(renderer, texture);
        
        for(int y=0;y<10;y++)
        {
            putPixel(renderer,40+x*10,50+y);
        }
        
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay( 100 );
    }
}

int get_image(){
    SDL_Window *window;
    SDL_Texture *texture;
    SDL_Renderer *renderer;

    window = SDL_CreateWindow("SDL2 Test", 0,0,SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    //SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND); // ???
    
    SDL_SetRenderTarget(renderer, texture);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // draw_test(window, renderer, texture);
    draw_bin(window, renderer, texture, "/Users/igridchyn/test-data/haibing/jc11/jc11-1704_20.BIN");
    //draw_bin(window, renderer, texture, "/Users/igridchyn/test-data/peter/jc85-2211-02checkaxona10m.bin.64.1");
    //draw_bin(window, renderer, texture, "/Users/igridchyn/Projects/sdl_example/bin/polarity.bin");
    SDL_Delay( 2000 );
    
    return 0;
}

int main( int argc, char* args[] )
{
    get_image();
    
    return 0;
}