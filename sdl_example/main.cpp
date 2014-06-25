#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "LFPProcessor.h"
#include "LFPPipeline.h"
#include "UnitTestingProcessor.h"
#include "PositionDisplayProcessor.h"
#include "AutocorrelogramProcessor.h"
#include "SpikeDetectorProcessor.h"
#include "PCAExtractionProcessor.h"
#include "GMMClusteringProcessor.h"
#include "SDLPCADisplayProcessor.h"
#include "PlaceFieldProcessor.h"
#include "FetFileReaderProcessor.h"
#include "CluReaderClusteringProcessor.h"
#include "WhlFileReaderProcessor.h"
#include "SpeedEstimationProcessor.h"
#include "SlowDownProcessor.h"
#include "KDClusteringProcessor.h"

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

    // clustering saved
//    tetr_inf->tetrodes_number = 5;
//    tetr_inf->channels_numbers = new int[5]{4, 4, 4, 4, 4};
////    tetr_inf->tetrode_channels = new int*[5]{new int[4]{32,33,34,35}, new int[4]{36,37,38,39}, new int[4]{40,41,42,43}, new int[4]{52,53,54,55}, new int[4]{60,61,62,63}};
//    tetr_inf->tetrode_channels = new int*[5]{new int[4]{4,5,6,7}, new int[4]{8,9,10,11}, new int[4]{12,13,14,15}, new int[4]{16,17,18,19}, new int[4]{20,21,22,23}};

    tetr_inf = TetrodesInfo::GetMergedTetrodesInfo(TetrodesInfo::GetInfoForTetrodesRange(0, 11), TetrodesInfo::GetInfoForTetrodesRange(13, 15));
//    tetr_inf = TetrodesInfo::GetInfoForTetrodesRange(0, 0);
    
    // in ms
    const unsigned int BUF_POP_VEC_WIN_LEN_MS = 100;
    const unsigned int BUF_SAMPLING_RATE = 20000;
    
    LFPBuffer *buf = new LFPBuffer(tetr_inf, BUF_POP_VEC_WIN_LEN_MS, BUF_SAMPLING_RATE);

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
    
    // PLACE FIELD PARAMS
    const double PF_SIGMA = 60.0f;
    const double PF_BIN_SIZE = 20;
    const unsigned int PF_NBINS = 20;
    const unsigned int PF_SPREAD = 1;
    const bool PF_LOAD = true;
    const bool PF_SAVE = false;
//    const std::string PF_BASE_PATH = "/hd1/data/bindata/jc103/0606/pf/pf_";
    const std::string PF_BASE_PATH = "/hd1/data/bindata/jc103/jc84/jc84-1910-0116/pf/pf_";
    const float PF_RREDICTION_FIRING_RATE_THRESHOLD = 0.3;
    const unsigned int PF_MIN_PKG_ID = 0;
    const bool PF_USE_PRIOR = false;
    
    const unsigned int SD_WAIT_MILLISECONDS = 150;
    const unsigned int SD_START = 300000000;

    // Position display params
    const unsigned int POS_TAIL_LENGTH = 300;

    // Autocorrelation display params
    const float AC_BIN_SIZE_MS = 1;
    const unsigned int AC_N_BINS = 30;

//    const char* filt_path = "/Users/igridchyn/Dropbox/IST_Austria/Csicsvari/Data Processing/spike_detection//filters/24k800-8000-50.txt";
    const char* filt_path = "/home/igor/code/ews/lfp_online/sdl_example/24k800-8000-50.txt";
//    pipeline->add_processor(new PackageExractorProcessor(buf));
//    pipeline->add_processor(new SpikeDetectorProcessor(buf, filt_path, DET_NSTD, 16));
//    pipeline->add_processor(new SpikeAlignmentProcessor(buf));
//    pipeline->add_processor(new WaveShapeReconstructionProcessor(buf, 4));
//    //pipeline->add_processor(new FileOutputProcessor(buf));
//    pipeline->add_processor(new PCAExtractionProcessor(buf, 3, 16, PCA_MIN_SAMPLES, PCA_LOAD_TRANSFORM, PCA_SAVE_TRANSFORM, "/hd1/data/bindata/jc103/0606/pca/pc_"));
//
    PlaceFieldProcessor *pfProc = new PlaceFieldProcessor(buf, PF_SIGMA, PF_BIN_SIZE, PF_NBINS, PF_SPREAD, PF_LOAD, PF_SAVE,
    		PF_BASE_PATH, PF_RREDICTION_FIRING_RATE_THRESHOLD, PF_MIN_PKG_ID, PF_USE_PRIOR);

    std::vector<int> tetrnums = Utils::Math::MergeRanges(Utils::Math::GetRange(1, 12), Utils::Math::GetRange(14,16));
    std::string dat_path_base = "/hd1/data/bindata/jc103/jc84/jc84-1910-0116/mjc84-1910-0116_4.";

    pipeline->add_processor(new WhlFileReaderProcessor(buf, dat_path_base + "whl", 512));
    pipeline->add_processor(new FetFileReaderProcessor(buf, dat_path_base +  "fet.", tetrnums));

//    pipeline->add_processor(new FetFileReaderProcessor(buf, "/Users/igridchyn/test-data/haibing/jc86/jc86-2612-01103.fet.9"));
//    pipeline->add_processor(new CluReaderClusteringProcessor(buf, dat_path_base +  + "clu.", dat_path_base +  +"res.", tetrnums));
    const std::string kd_path_base = "/hd1/data/bindata/jc103/jc84/jc84-1910-0116/pf_ws/lax1/pf_";
    pipeline->add_processor(new KDClusteringProcessor(buf, 20000, kd_path_base, pfProc));

    pipeline->add_processor(new SpeedEstimationProcessor(buf));

    pipeline->add_processor(new SlowDownProcessor(buf, SD_WAIT_MILLISECONDS, SD_START));

//    GMMClusteringProcessor *gmmClustProc = new GMMClusteringProcessor(buf, GMM_MIN_OBSERVATIONS, GMM_RATE, GMM_MAX_CLUSTERS, GMM_LOAD_MODELS, GMM_SAVE_MODELS, "/hd1/data/bindata/jc103/0606/clust/gmm_");
//    pipeline->add_processor(gmmClustProc);
    
//    pipeline->add_processor( new SDLSignalDisplayProcessor(buf, "LFP", 1280, 600, 4, new unsigned int[4]{0, 1, 2, 3}) );
//    pipeline->add_processor(new SDLPCADisplayProcessor(buf, "PCA", 800, 600, 0, DISPLAY_UNCLASSIFIED, .5, 300));
    
    // TESTING: jc11-1704_20.BIN, 8-11 channels; 2 PCs from channel 8
    //pipeline->add_processor(new UnitTestingProcessor(buf, std::string("/Users/igridchyn/Projects/sdl_example/unit_tests/")));
    
    pipeline->add_processor(new PositionDisplayProcessor(buf, "Tracking", 450, 450, 0, POS_TAIL_LENGTH));
    
    //pipeline->add_processor(new FrequencyPowerBandProcessor(buf, "Power Frequency Band", 1600, 600));
    
//    pipeline->add_processor(new SDLWaveshapeDisplayProcessor(buf, "Waveshapes", 127*4+1, 800));
    
//    pipeline->add_processor(new AutocorrelogramProcessor(buf, AC_BIN_SIZE_MS, AC_N_BINS));
    
    pipeline->add_processor(pfProc);
    
    // should be added after all control processor
    pipeline->add_processor(new SDLControlInputMetaProcessor(buf, pipeline->GetSDLControlInputProcessors()));
    
    // TODO: parallel threads ?
    while(!feof(f))
    {
        fread((void*)block, CHUNK_SIZE, 1, f);

        buf->chunk_ptr = block;
        buf->num_chunks = 1;
        
        pipeline->process(block, 1);
    }
    
    buf->chunk_ptr = NULL;
    while(true){
        pipeline->process(block, 1);
    }
    
    std::cout << "EOF, waiting for clustering jobs...\n";
//    gmmClustProc->JoinGMMTasks();
}

int main( int argc, char* args[] )
{
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
//    draw_bin("/Users/igridchyn/data/bindata/jc103/jc103-0106_03l.bin");
    
//    draw_bin("/run/media/igor/63ce153c-da52-47cf-b229-f0bc4078cd52/data/bindata/jc58/trial100.bin");
//	draw_bin("/hd1/data/bindata/jc103/0606/jc103-0606_03l.bin");
	draw_bin("/home/igor/tmp/jc103-0606_03l.bin");
    
    return 0;
}
