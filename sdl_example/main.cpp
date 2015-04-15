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
	// std::string cpath(R"(d:\Igor\soft\lfp_online\sdl_example\Res\signal_display_jc117_0919_4l_win.conf)");

		// 1) dump spikes
		// std::string cpath(R"(../Res/spike_dump_win.conf)");
		// 2) diplay
		std::string cpath(R"(../Res/spike_display_win.conf)");

		//std::string cpath(R"(../Res/delay_test_win.conf)");
		//std::string cpath(R"(../Res/spike_detection_build_model_jc118_1003_8l_shift_WIN.conf)");
		//std::string cpath(R"(../Res/decoding_online_jc118_1003_shift_WIN.conf)");
		//std::string cpath(R"(../Res/trigger_jc140_win.conf)");
		//std::string cpath(R"(../Res/spike_detection_build_model_jc140_WIN.conf)");
		//std::string cpath(R"(../Res/decoding_online_jc140_win.conf)");

		//system("d:/Igor/soft/lfp_online/sdl_example/lfponlinevs/Release/kde_win.exe");

		Config *config = nullptr;

		if (argc > 1){
			size_t convertedChars = 0;
			size_t origsize = wcslen(argv[1]) + 1;
			const size_t newsize = origsize*2;
			char *nstring = new char[newsize];
			wcstombs_s(&convertedChars, nstring, newsize, argv[1], _TRUNCATE);

			config = new Config(nstring);

			std::cout << "Read config: " << nstring << "\n";
		}
		else {
			config = new Config(cpath);
		}

	
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
    Config *config = nullptr;

    if (argc > 1){
    	config = new Config(argv[1]);
    } else {
//	Config *config = new Config("../Res/spike_detection_build_model_jc118_1003_8l.conf");
//    	config = new Config("../Res/decoding_online_jc118_1003.conf");
//    	config = new Config("../Res/spike_detection_build_model_jc118_1003_8l_shift.conf");
    	config = new Config("../Res/decoding_online_jc118_1003_shift.conf");
//    	config = new Config("../Res/spike_display_jc118_1003.conf");
//    	config = new Config("../Res/spike_dump.conf");
//    	config = new Config("../Res/spike_display.conf");
//    	config = new Config("../Res/delay_test.conf");
//    	config = new Config("../Res/trigger_jc140.conf");
//    	config = new Config("../Res/spike_detection_build_model_jc140.conf");
//    	config = new Config("../Res/decoding_online_jc140.conf");

//    Config *config = new Config("../Res/spike_dump_128.conf");
//      Config *config = new Config("../Res/spike_display_128.conf");

//	Config *config = new Config("../Res/build_model_jc118_1003_env_shift.conf"); // build model for whl with corrds of one environment shifted by the arena width
//	Config *config = new Config("../Res/decoding_jc118_1003_env1_2x.cz`onf"); // shifted map
//	Config *config = new Config("../Res/decoding_jc118_1003_both_env_swr_2x.conf"); // swr decoding in the shfited map

//	Config *config = new Config("../Res/spike_reader_jc118_1002_10s.conf");
//	Config *config = new Config("../Res/spike_detection_jc118_1003_3l.conf");

    }

#endif

	LFPBuffer *buf = new LFPBuffer(config);
	LFPPipeline *pipeline = new LFPPipeline(buf);

	while (true) {
		pipeline->process();
	}

	return 0;
}
