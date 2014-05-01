#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>
#include "LFPProcessor.h"
#include "UnitTestingProcessor.h"
#include "PositionDisplayProcessor.h"

void putPixel(SDL_Renderer *renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    SDL_RenderDrawPoint(renderer, x, y);
}

typedef short t_bin;

void draw_bin(const char *path){
    FILE *f = fopen(path, "rb");
    
    const int CHUNK_SIZE = 432; // bytes
    
    unsigned char block[ CHUNK_SIZE ];
    
    TetrodesInfo *tetr_inf = new TetrodesInfo();
    tetr_inf->tetrodes_number = 1;
    tetr_inf->channels_numbers = new int[1]{4};
    tetr_inf->tetrode_channels = new int*[1]{new int[4]{8,9,10,11}};
    
    LFPBuffer *buf = new LFPBuffer(tetr_inf);

    LFPPipeline *pipeline = new LFPPipeline();
    SDLSignalDisplayProcessor *sdlSigDispProc = new SDLSignalDisplayProcessor(buf, "LFP", 1280, 600, 0);
    
    const char* filt_path = "/Users/igridchyn/Dropbox/IST_Austria/Csicsvari/Data Processing/spike_detection//filters/24k800-8000-50.txt";
    pipeline->add_processor(new PackageExractorProcessor(buf));
    pipeline->add_processor(new SpikeDetectorProcessor(buf, filt_path, 5.0, 16));
    pipeline->add_processor(new SpikeAlignmentProcessor(buf));
    pipeline->add_processor(new WaveShapeReconstructionProcessor(buf, 4));
    pipeline->add_processor(new FileOutputProcessor(buf));
    pipeline->add_processor(new PCAExtractionProcessor(buf, 3, 16));
    pipeline->add_processor(new GMMClusteringProcessor(buf));
    //pipeline->add_processor(sdlSigDispProc);
    pipeline->add_processor(new SDLControlInputMetaProcessor(buf, sdlSigDispProc));
    pipeline->add_processor(new SDLPCADisplayProcessor(buf, "PCA", 800, 600));
    pipeline->add_processor(new UnitTestingProcessor(buf, std::string("/Users/igridchyn/Projects/sdl_example/unit_tests/")));
    pipeline->add_processor(new PositionDisplayProcessor(buf, "Tracking", 600, 600));
    
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
    // draw_test(window, renderer, texture);
    draw_bin("/Users/igridchyn/test-data/haibing/jc11/jc11-1704_20.BIN");
    //draw_bin(window, renderer, texture, "/Users/igridchyn/test-data/peter/jc85-2211-02checkaxona10m.bin.64.1");
    //draw_bin(window, renderer, texture, "/Users/igridchyn/Projects/sdl_example/bin/polarity.bin");
    // SDL_Delay( 2000 );
    char c = getchar();
    
    return 0;
}

int main( int argc, char* args[] )
{
    get_image();
    
    return 0;
}