#include <SDL2/SDL.h>
//#include <SDL2/SDL_ttf.h>

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
#include "TransProbEstimationProcessor.h"
#include "SwReaderProcessor.h"
#include "Config.h"

typedef short t_bin;

// CONTROLS:
//
// LALT+NUM - switch display tetrode
// CTRL+NUM - switch controlled window
// NUM - switch PCA dimensions / cluster waveshape
// NUMPAD NUM - switch PCA dismension
// ESC - exit
// SHIFT+NUM ~ 10+NUM

void draw_bin() {
	Config *config = new Config("../Res/decoding_32_jc84.conf");
	const char* path = config->getString("bin.path").c_str();
	FILE *f = fopen(path, "rb");

	const int CHUNK_SIZE = config->getInt("chunk.size"); // bytes

	unsigned char block[CHUNK_SIZE];

	TetrodesInfo *tetr_inf = new TetrodesInfo(config->getString("tetr.conf.path"));

	LFPBuffer *buf = new LFPBuffer(tetr_inf, config);

	// Position display params
	const unsigned int POS_TAIL_LENGTH = config->getInt("pos.tail.length");

	LFPPipeline *pipeline = new LFPPipeline();

//    const char* filt_path = "/Users/igridchyn/Dropbox/IST_Austria/Csicsvari/Data Processing/spike_detection//filters/24k800-8000-50.txt";
	const char* filt_path = config->getString("spike.detection.filter.path").c_str();
//    pipeline->add_processor(new PackageExractorProcessor(buf));
//    pipeline->add_processor(new SpikeDetectorProcessor(buf));
//    pipeline->add_processor(new SpikeAlignmentProcessor(buf));
//    pipeline->add_processor(new WaveShapeReconstructionProcessor(buf, 4));
//    //pipeline->add_processor(new FileOutputProcessor(buf));
//    pipeline->add_processor(new PCAExtractionProcessor(buf));
//
	PlaceFieldProcessor *pfProc = new PlaceFieldProcessor(buf);

	std::vector<int> tetrnums = Utils::Math::MergeRanges(
			Utils::Math::GetRange(1, 12), Utils::Math::GetRange(14, 16));
	std::string dat_path_base = config->getString("dat.path.base");

	pipeline->add_processor(new WhlFileReaderProcessor(buf));
	pipeline->add_processor(
			new FetFileReaderProcessor(buf, dat_path_base + "fet.", tetrnums));
	pipeline->add_processor(new SwReaderProcessor(buf, dat_path_base + "answ"));
	KDClusteringProcessor *kdClustProc = new KDClusteringProcessor(buf);
	pipeline->add_processor(kdClustProc);
	pipeline->add_processor(new SpeedEstimationProcessor(buf));
	pipeline->add_processor(new TransProbEstimationProcessor(buf));
	pipeline->add_processor(new SlowDownProcessor(buf));

//    GMMClusteringProcessor *gmmClustProc = new GMMClusteringProcessor(buf);
//    pipeline->add_processor(gmmClustProc);
//    pipeline->add_processor( new SDLSignalDisplayProcessor(buf, "LFP", 1280, 600, 4, new unsigned int[4]{0, 1, 2, 3}) );
//    pipeline->add_processor(new SDLPCADisplayProcessor(buf, "PCA", 800, 600, 0, DISPLAY_UNCLASSIFIED, .5, 300));
	// TESTING: jc11-1704_20.BIN, 8-11 channels; 2 PCs from channel 8
	//pipeline->add_processor(new UnitTestingProcessor(buf, std::string("/Users/igridchyn/Projects/sdl_example/unit_tests/")));
//    pipeline->add_processor(new PositionDisplayProcessor(buf, "Tracking", 450, 450, 0, POS_TAIL_LENGTH));
	//pipeline->add_processor(new FrequencyPowerBandProcessor(buf, "Power Frequency Band", 1600, 600));
//    pipeline->add_processor(new SDLWaveshapeDisplayProcessor(buf, "Waveshapes", 127*4+1, 800));
//    pipeline->add_processor(new AutocorrelogramProcessor(buf));

	pipeline->add_processor(pfProc);

	// should be added after all control processor
	pipeline->add_processor(
			new SDLControlInputMetaProcessor(buf,
					pipeline->GetSDLControlInputProcessors()));

	// check for unused params in the config
	config->checkUnused();

	// TODO: parallel threads ?
	while (!feof(f)) {
		fread((void*) block, CHUNK_SIZE, 1, f);

		buf->chunk_ptr = block;
		buf->num_chunks = 1;

		pipeline->process(block, 1);
	}

	std::cout << "Out of data packages, entering endless loop to process user input. Press ESC to exit...\n";

	buf->chunk_ptr = NULL;
	while (true) {
		pipeline->process(block, 1);
	}

	std::cout << "EOF, waiting for processors jobs to join...\n";

	kdClustProc->JoinKDETasks();
//    gmmClustProc->JoinGMMTasks();
}

int main(int argc, char* args[]) {
	// draw_test(window, renderer, texture);

	//TEST DATA; for this channels: 8-11 : 2 clear CLUSTERS; fixed THRESHOLD !
//    draw_bin("/Users/igridchyn/test-data/haibing/jc11/jc11-1704_20.BIN");

//    draw_bin("/Users/igridchyn/test-data/haibing/jc11/1403-1406/jc11-1t204_01.BIN");

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
	draw_bin();

	return 0;
}
