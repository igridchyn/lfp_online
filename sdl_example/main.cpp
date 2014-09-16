#include "LFPProcessor.h"
#include "LFPPipeline.h"
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
#ifdef _WIN32
	std::string conf_path = "../Res/spike_detection_jc103_win.conf";
	//Config *config = new Config(conf_path);
	Config *config = new Config("../Res/signal_display_win2.conf");
	//FILE *f = fopen("D:/data/igor/jc103-2705_02l.bin", "rb");
	//FILE *f = fopen("D:/data/igor/test1.bin", "rb");
	FILE *f = fopen("D:/igor/data/square.bin", "rb");
	//FILE *f = fopen("d:/Igor/data/jc117_0914_screen3.bin", "rb");
#elif defined(__APPLE__)
    // square wave signal - for delay and stability testing
//    Config *config = new Config("../Res/signal_display_mac.conf");
    
    // decoding position with best params - from fet files
    Config *config = new Config("../Res/decoding_32_jc84_mac.conf");
    
    //	Config *config = new Config("../Res/spike_detection_jc11.conf");
	FILE *f = fopen(config->getString("bin.path").c_str(), "rb");
#else
//	Config *config = new Config("../Res/build_model_jc84.conf");
//	Config *config = new Config("../Res/build_model_jc84_2110.conf");
//	Config *config = new Config("../Res/decoding_32_jc84.conf");
//	Config *config = new Config("../Res/decoding_32_jc84_2110.conf");
//	Config *config = new Config("../Res/spike_detection_and_KD_jc103.conf");
//	Config *config = new Config("../Res/spike_detection_jc103_nodisp.conf");
//	Config *config = new Config("../Res/spike_detection_jc103.conf");
//	Config *config = new Config("../Res/spike_detection_jc117_load_pc_gmm.conf");
//	Config *config = new Config("../Res/spike_detection_jc117_0908_screen2.conf");
//	Config *config = new Config("../Res/spike_detection_jc117_0908_screen2_npc2_step2.conf");
//	Config *config = new Config("../Res/jc117_power.conf");
//	Config *config = new Config("../Res/spike_detection_jc117_0911_screen2.conf");
//	Config *config = new Config("../Res/spike_detection_jc117_0914_screen3.conf");
//	Config *config = new Config("../Res/nocon.conf");
	Config *config = new Config("../Res/spike_detection_jc117_0916_screen0.conf");

//	Config *config = new Config("../Res/signal_display.conf");
//	Config *config = new Config("../Res/spike_detection_jc11.conf");
	std::string binpath = config->getString("bin.path");
	if (!Utils::FS::FileExists(binpath)){
		std::cout << "File doesn't exist: " << binpath << "\n";
		exit(1);
	}
	FILE *f = fopen(binpath.c_str(), "rb");
#endif

	const int CHUNK_SIZE = config->getInt("chunk.size"); // bytes

	const unsigned int nblock = 1;
	unsigned char *block = new unsigned char[CHUNK_SIZE * nblock];

	LFPBuffer *buf = new LFPBuffer(config);
	LFPPipeline *pipeline = new LFPPipeline(buf);

	// TEST
//	delete pipeline;
//	delete buf;
//	buf = new LFPBuffer(config);
//	pipeline = new LFPPipeline(buf);

	// check for unused parameters in the config
//	config->checkUnused();

	// TODO: parallel threads ?
	while (!feof(f)) {
		fread((void*)block, 1, CHUNK_SIZE*nblock, f);

		buf->chunk_ptr = block;
		buf->num_chunks = nblock;
		pipeline->process(NULL);
	}

	std::cout << "Out of data packages, entering endless loop to process user input. Press ESC to exit...\n";

	buf->chunk_ptr = NULL;
	while (true) {
		pipeline->process(NULL);
	}

	std::cout << "EOF, waiting for processors jobs to join...\n";

	// TODO: perfrom from within pipeline (virtual 'End tasks' ?)
//	kdClustProc->JoinKDETasks();
//    gmmClustProc->JoinGMMTasks();
}

#ifdef WIN32
	int wmain(int argc, wchar_t *argv[]){
#else // WIN32
	int main(int argc, char *argv[]){
#endif // WIN32
	draw_bin();

	return 0;
}
