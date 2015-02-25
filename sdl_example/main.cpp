#include "LFPProcessor.h"
#include "LFPPipeline.h"
#include "Config.h"
#include "boost/filesystem.hpp"

typedef short t_bin;

// CONTROLS:
//
// LALT+NUM - switch display tetrode
// CTRL+NUM - switch controlled window
// NUM - switch PCA dimensions / cluster waveshape
// NUMPAD NUM - switch PCA dismension
// ESC - exit
// SHIFT+NUM ~ 10+NUM

#ifdef WIN32
	int wmain(int argc, wchar_t *argv[]){
#else // WIN32
	int main(int argc, char *argv[]){
#endif // WIN32

#ifdef _WIN32
	//std::string cpath("../Res/signal_display_win_bin.conf");
	std::string cpath(R"(d:\Igor\soft\lfp_online\sdl_example\Res\signal_display_jc117_0919_4l_win.conf)");
	Config *config = new Config(cpath);
	//config->Init();
#elif defined(__APPLE__)
    // square wave signal - for delay and stability testing
//    Config *config = new Config("../Res/signal_display_mac.conf");
    
//    Config *config = new Config("../Res/decoding_32_jc84_mac.conf");
//    Config *config = new Config("../Res/decoding_jc118_1003_env2_mac.conf");
//	  Config *config = new Config("../Res/decoding_jc118_1003_env1_2x_MAC.conf");
    Config *config = new Config("../Res/spike_detection_build_model_jc118_1003_8l_shift_MAC.conf");

    //	Config *config = new Config("../Res/spike_detection_jc11.conf");
#else
//	Config *config = new Config("../Res/spike_detection_build_model_jc118_1003_8l.conf");
//    Config *config = new Config("../Res/decoding_online_jc118_1003.conf");
//    Config *config = new Config("../Res/spike_detection_build_model_jc118_1003_8l_shift.conf");
    	Config *config = new Config("../Res/decoding_online_jc118_1003_shift.conf");

    if (argc > 1){
    	delete config;
    	config = new Config(argv[1]);
    }

//	Config *config = new Config("../Res/build_model_jc118_1003_env_shift.conf"); // build model for whl with corrds of one environment shifted by the arena width
//	Config *config = new Config("../Res/decoding_jc118_1003_env1_2x.conf"); // shifted map
//	Config *config = new Config("../Res/decoding_jc118_1003_both_env_swr_2x.conf"); // swr decoding in the shfited map

//	Config *config = new Config("../Res/spike_reader_jc118_1002_10s.conf");
//	Config *config = new Config("../Res/spike_detection_jc118_1003_3l.conf");

#endif

	LFPBuffer *buf = new LFPBuffer(config);
	LFPPipeline *pipeline = new LFPPipeline(buf);

	while (true) {
		pipeline->process();
	}

	return 0;
}
