#include <SDL2/SDL.h>
//#include <SDL2/SDL_ttf.h>

#include <tchar.h>

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
	// Config *config = new Config("../Res/decoding_32_jc84.conf");
	Config *config = new Config("Res/spike_detection_jc103.conf");

	const char* path = config->getString("bin.path").c_str();
	FILE *f = fopen(path, "rb");

	const int CHUNK_SIZE = config->getInt("chunk.size"); // bytes

	unsigned char *block = new unsigned char[CHUNK_SIZE];

	TetrodesInfo *tetr_inf = new TetrodesInfo(config->getString("tetr.conf.path"));

	LFPBuffer *buf = new LFPBuffer(tetr_inf, config);

	LFPPipeline *pipeline = new LFPPipeline(buf);

	// check for unused parameters in the config
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

	// TODO: perfrom from within pipeline (virtual 'End tasks' ?)
//	kdClustProc->JoinKDETasks();
//    gmmClustProc->JoinGMMTasks();
}

#ifdef WIN32
int wmain(int argc, wchar_t *argv[]){
#elif // WIN32
int main(int argc, wchar_t *argv[]){
#endif // WIN32
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
