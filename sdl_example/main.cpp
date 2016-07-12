#include "LFPProcessor.h"
#include "LFPPipeline.h"
#include "Config.h"
#include "boost/filesystem.hpp"
#include <condition_variable>

#include "BinFileReaderProcessor.h"

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
void usleep(__int64 usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(nullptr, TRUE, nullptr);
	SetWaitableTimer(timer, &ft, 0, nullptr, nullptr, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

	int wmain(int argc, wchar_t *argv[]){
#else // WIN32
	int main(int argc, char *argv[]){
#endif // WIN32

#ifdef _WIN32

		// 0) delay test
		//std::string cpath(R"(../Res/delay_test_win.conf)");
		// 1) dump spikes
		//std::string cpath(R"(../Res/spike_dump_win.conf)");
		// 2) diplay
		//std::string cpath(R"(../Res/spike_display_win.conf)");
		// 3) build model
		//std::string cpath(R"(../Res/build_model_win.conf)");
		// 4) test model - decode online
		//std::string cpath(R"(../Res/decoding_online_win.conf)");
		// 5) test synchrony detection
		//std::string cpath(R"(../Res/synchrony_detection_win.conf)");
		// 4) detect assemblies online
		std::string cpath(R"(../Res/assembly_inhibition_win.conf)");
		
		Config *config = nullptr;

		if (argc > 1){
			char *nstring = Utils::Converter::WstringToCstring(argv[1]);

			if (argc > 2){
				char **params = new char*[argc - 2];
				for (int p=0; p < argc - 2; ++p){
					params[p] = Utils::Converter::WstringToCstring(argv[p + 2]);
				}
				config = new Config(nstring, argc - 2, params);
			}else{
				config = new Config(nstring);
			}

			std::cout << "Read config: " << nstring << "\n";
		}
		else {
			config = new Config(cpath);
		}

	
	//config->Init();
#elif defined(__APPLE__)
    // square wave signal - for delay and stability testing
//    Config *config = new Config("../Res/signal_display_mac.conf");
#else
    Config *config = nullptr;

    if (argc > 1){
    	config = new Config(argv[1], argc - 2, argv + 2);
    } else {

//    	config = new Config("../Res/assembly_inhibition.conf");
//    	config = new Config("../Res/build_model.conf");
//    	config = new Config("../Res/decoding_online.conf");
    	config = new Config("../Res/spike_display.conf");
//    	config = new Config("../Res/spike_dump.conf");
//    	config = new Config("../Res/synchrony_detection.conf");
    }

#endif

	LFPBuffer *buf = new LFPBuffer(config);
	LFPPipeline *pipeline = new LFPPipeline(buf);

#ifdef PIPELINE_THREAD
	BinFileReaderProcessor *binreader = new BinFileReaderProcessor(buf);
#endif

	while (!buf->processing_over_) {
#ifdef PIPELINE_THREAD
		{
			std::lock_guard<std::mutex> lk(pipeline->mtx_data_add_);
			binreader->process();
			pipeline->data_added_ = true;
		}
		pipeline->cv_data_added_.notify_one();
		//wait to simulate real-time [1 25 us for 24 kHz]
		usleep(10);
#else
		pipeline->process();
#endif
	}

	delete config;
	delete pipeline;
	delete buf;

	std::cout << "PROCESSING OVER; CONFIG, PIPELINE AND BUFFER DELETED\n";
	// usleep(30000000);

	return 0;
}
