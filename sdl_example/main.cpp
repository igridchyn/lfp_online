#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>
#include "LFPProcessor.h"
#include "UnitTestingProcessor.h"
#include "PositionDisplayProcessor.h"
#include "AutocorrelogramProcessor.h"
#include "SpikeDetectorProcessor.h"
#include "PCAExtractionProcessor.h"
#include "GMMClusteringProcessor.h"
#include "SDLPCADisplayProcessor.h"
#include "PlaceFieldProcessor.h"

void putPixel(SDL_Renderer *renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    SDL_RenderDrawPoint(renderer, x, y);
}

typedef short t_bin;

// CONTROLS:
//
// LALT+NUM - switch display tetrode
// CTRL+NUM - switch controlled window
// NUM - switch PCA dimensions / cluster waveshape
// NUMPAD NUM - switch PCA dismension
// ESC - exit
// SHIFT+NUM ~ 10+NUM

void draw_bin(const char *path){
    FILE *f = fopen(path, "rb");
    
    const int CHUNK_SIZE = 432; // bytes
    
    unsigned char block[ CHUNK_SIZE ];
    
    TetrodesInfo *tetr_inf = new TetrodesInfo();
    
//    tetr_inf->tetrodes_number = 2;
//    tetr_inf->channels_numbers = new int[2]{4, 4};
//    tetr_inf->tetrode_channels = new int*[2]{new int[4]{8,9,10,11}, new int[4]{16,17,18,19}};

//    tetr_inf->tetrodes_number = 2;
//    tetr_inf->channels_numbers = new int[2]{4, 4};
//    tetr_inf->tetrode_channels = new int*[2]{new int[4]{44,45,46,47}, new int[4]{48,49,50,51}};
    
//    tetr_inf->tetrodes_number = 5;
//    tetr_inf->channels_numbers = new int[5]{4, 4, 4, 4, 4};
//    tetr_inf->tetrode_channels = new int*[5]{new int[4]{8,9,10,11}, new int[4]{16,17,18,19}, new int[4]{20,21,22,23}, new int[4]{24,25,26,27}, new int[4]{28,29,30,31}};

    // clustering saved
    tetr_inf->tetrodes_number = 5;
    tetr_inf->channels_numbers = new int[5]{4, 4, 4, 4, 4};
//    tetr_inf->tetrode_channels = new int*[5]{new int[4]{32,33,34,35}, new int[4]{36,37,38,39}, new int[4]{40,41,42,43}, new int[4]{52,53,54,55}, new int[4]{60,61,62,63}};
    tetr_inf->tetrode_channels = new int*[5]{new int[4]{4,5,6,7}, new int[4]{8,9,10,11}, new int[4]{12,13,14,15}, new int[4]{16,17,18,19}, new int[4]{20,21,22,23}};
    
//    tetr_inf->tetrodes_number = 1;
//    tetr_inf->channels_numbers = new int[1]{4};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{0,1,2,3}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{4,5,6,7}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{8,9,10,11}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{12,13,14,15}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{16,17,18,19}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{20,21,22,23}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{24,25,26,27}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{28,29,30,31}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{32,33,34,35}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{36,37,38,39}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{40,41,42,43}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{44,45,46,47}}; // +
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{48,49,50,51}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{52,53,54,55}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{56,57,58,59}};
//    tetr_inf->tetrode_channels = new int*[1]{new int[4]{60,61,62,63}};
    
    LFPBuffer *buf = new LFPBuffer(tetr_inf);

    LFPPipeline *pipeline = new LFPPipeline();
    
    // DETECTION PARA<S
    const float DET_NSTD = 6.5;
    
    // CLUSTERING PARAMS
    const unsigned int GMM_MIN_OBSERVATIONS = 20000;
    const unsigned int GMM_RATE = 1;
    const unsigned int GMM_MAX_CLUSTERS = 18;
    const bool GMM_LOAD_MODELS = true;
    const bool GMM_SAVE_MODELS = false;
    
    // PCA PARAMS
    const unsigned int PCA_MIN_SAMPLES = 10000;
    const bool DISPLAY_UNCLASSIFIED = false;
    const bool PCA_LOAD_TRANSFORM = true;
    const bool PCA_SAVE_TRANSFORM = false;
    
    const char* filt_path = "/Users/igridchyn/Dropbox/IST_Austria/Csicsvari/Data Processing/spike_detection//filters/24k800-8000-50.txt";
    pipeline->add_processor(new PackageExractorProcessor(buf));
    pipeline->add_processor(new SpikeDetectorProcessor(buf, filt_path, DET_NSTD, 16));
    pipeline->add_processor(new SpikeAlignmentProcessor(buf));
    pipeline->add_processor(new WaveShapeReconstructionProcessor(buf, 4));
    //pipeline->add_processor(new FileOutputProcessor(buf));
    pipeline->add_processor(new PCAExtractionProcessor(buf, 3, 16, PCA_MIN_SAMPLES, PCA_LOAD_TRANSFORM, PCA_SAVE_TRANSFORM));
    
    //pipeline->add_processor(new FetReaderProcessor(buf, "/Users/igridchyn/test-data/haibing/BIN/jc22-0507-0115.fet.4"));
    //pipeline->add_processor(new FetReaderProcessor(buf, "/Users/igridchyn/test-data/haibing/jc86/jc86-2612-01103.fet.9"));
    
    GMMClusteringProcessor *gmmClustProc = new GMMClusteringProcessor(buf, GMM_MIN_OBSERVATIONS, GMM_RATE, GMM_MAX_CLUSTERS, GMM_LOAD_MODELS, GMM_SAVE_MODELS);
    pipeline->add_processor(gmmClustProc);
//    pipeline->add_processor( new SDLSignalDisplayProcessor(buf, "LFP", 1280, 600, 4, new unsigned int[4]{0, 1, 2, 3}) );
    pipeline->add_processor(new SDLPCADisplayProcessor(buf, "PCA", 800, 600, 0, DISPLAY_UNCLASSIFIED));
    
    // TESTING: jc11-1704_20.BIN, 8-11 channels; 2 PCs from channel 8
    //pipeline->add_processor(new UnitTestingProcessor(buf, std::string("/Users/igridchyn/Projects/sdl_example/unit_tests/")));
    
    pipeline->add_processor(new PositionDisplayProcessor(buf, "Tracking", 450, 450, 0));
    
    //pipeline->add_processor(new FrequencyPowerBandProcessor(buf, "Power Frequency Band", 1600, 600));
    
//    pipeline->add_processor(new SDLWaveshapeDisplayProcessor(buf, "Waveshapes", 127*4+1, 800));
    
//    pipeline->add_processor(new AutocorrelogramProcessor(buf));
    
    pipeline->add_processor(new PlaceFieldProcessor(buf, 40, 20, 30, 1));
    
    // should be added after all control porcessor
    pipeline->add_processor(new SDLControlInputMetaProcessor(buf, pipeline->GetSDLControlInputProcessors()));
    
    // TODO: parallel threads ?
    while(!feof(f))
    {
        fread((void*)block, CHUNK_SIZE, 1, f);
        
        buf->chunk_ptr = block;
        buf->num_chunks = 1;
        
        pipeline->process(block, 1);
        continue;

    }
    
    std::cout << "EOF, waiting for clustering jobs...\n";
    gmmClustProc->JoinGMMTasks();
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
    
    //TEST DATA; for this channels: 8-11 : 2 clear CLUSTERS; fixed THRESHOLD !
//    draw_bin("/Users/igridchyn/test-data/haibing/jc11/jc11-1704_20.BIN");
    
//    draw_bin("/Users/igridchyn/test-data/haibing/jc11/1403-1406/jc11-1204_01.BIN");
    
//    draw_bin("/Users/igridchyn/test-data/peter/jc85-2211-02checkaxona10m.bin.64.1");
    //draw_bin(window, renderer, texture, "/Users/igridchyn/Projects/sdl_example/bin/polarity.bin");
    
    // many units
//    draw_bin("/Users/igridchyn/test-data/haibing/jc86/jc86-2612_01.bin");
    
    // jc-103, screening
    // 12th tetrode
//    draw_bin("/Users/igridchyn/data/bindata/jc103/jc103-2305_02_explore.bin");
    
    // SAVED CLUSTERING AVAILABLE, tetrodes in layer: 1,3,(4),5,6,7,9,10
//    draw_bin("/Users/igridchyn/data/bindata/jc103/jc103-2705_02l.bin");
    draw_bin("/Users/igridchyn/data/bindata/jc103/jc103-0106_03l.bin");
    
    return 0;
}

int main( int argc, char* args[] )
{
    get_image();
    
    return 0;
}