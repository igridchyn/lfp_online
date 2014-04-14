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
    int CHANNEL = 1; //32
    const int HEADER_LEN = 32; // bytes
    const int BLOCK_SIZE = 64 * 2; // bytes
    
    unsigned char block[ CHUNK_SIZE ];
    const int CH_MAP[] = {32, 33, 34, 35, 36, 37, 38, 39,0, 1, 2, 3, 4, 5, 6, 7,40, 41, 42, 43, 44, 45, 46, 47,8, 9, 10, 11, 12, 13, 14, 15,48, 49, 50, 51, 52, 53, 54, 55,16, 17, 18, 19, 20, 21, 22, 23,56, 57, 58, 59, 60, 61, 62, 63,24, 25, 26, 27, 28, 29, 30, 31};
    
    int val_prev = 1;
    int x_prev = 1;
    int plot_hor_scale = 10;
    int plot_scale = 40;
    const int SHIFT = 11000;
    
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
    
    LFPBuffer *buf = new LFPBuffer();

    LFPPipeline *pipeline = new LFPPipeline();
    pipeline->add_processor(new PackageExractorProcessor(buf));
    pipeline->add_processor(new SDLSignalDisplayProcessor(buf, window, renderer, texture, 0));
    
    for (int i = 0; i < 1000000; ++i){
        fread((void*)block, CHUNK_SIZE, 1, f);
        
        buf->chunk_ptr = block;
        buf->num_chunks = 1;
        
        pipeline->process(block, 1);
        continue;
        
        // iterate throug 3 batches in 1 chunk
        for (int batch = 0; batch < 3; ++batch){

            t_bin *ch_dat =  (t_bin*)(block + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[CHANNEL]);
            int val = *ch_dat;
            int pack_num = *((int*)block + 1);
            
            printf("%d - %d\n", val, (int)pack_num);
            
            // scale for plotting
            val = val + SHIFT;
            val = val > 0 ? val / plot_scale : 1;
            val = val < SCREEN_HEIGHT ? val : SCREEN_HEIGHT;

            int sample_number = i*3 + batch;
            if (sample_number % plot_hor_scale == 0){
                SDL_SetRenderTarget(renderer, texture);
                drawLine(renderer, x_prev, val_prev, x_prev + 1, val);
                
                // render every N samples
                if (sample_number % (30 * plot_hor_scale) == 0){
                    SDL_SetRenderTarget(renderer, NULL);
                    SDL_RenderCopy(renderer, texture, NULL, NULL);
                    SDL_RenderPresent(renderer);
                }
                
                val_prev = val;
                x_prev++;
                if (x_prev == SCREEN_WIDTH - 1){
                    x_prev = 1;
                    
                    // reset screen
                    SDL_SetRenderTarget(renderer, texture);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderClear(renderer);
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    SDL_RenderDrawLine(renderer, 1, SHIFT/plot_scale, SCREEN_WIDTH, SHIFT/plot_scale);
                    SDL_RenderPresent(renderer);
                }
            }
            
            SDL_Event e;
            bool quit = false;
            while( SDL_PollEvent( &e ) != 0 )
            {
                //User requests quit
                if( e.type == SDL_QUIT )
                {
                    quit = true;
                }
                //User presses a key
                else if( e.type == SDL_KEYDOWN )
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
                            return;
                            break;
                        case SDLK_1:
                            CHANNEL = 1;
                            break;
                        case SDLK_2:
                            CHANNEL = 5;
                            break;
                        case SDLK_3:
                            CHANNEL = 9;
                            break;
                        case SDLK_4:
                            CHANNEL = 13;
                            break;
                        case SDLK_5:
                            CHANNEL = 17;
                            break;
                    }
                }
            }
        }
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
    draw_bin(window, renderer, texture, "/Users/igridchyn/test-data/peter/jc85-2211-02checkaxona10m.bin.64.1");
    //draw_bin(window, renderer, texture, "/Users/igridchyn/Projects/sdl_example/bin/polarity.bin");
    SDL_Delay( 2000 );
    
    return 0;
}

int main( int argc, char* args[] )
{
    get_image();
    
    return 0;
}