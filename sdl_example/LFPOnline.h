#ifndef sdl_example_LFPOnline_h
#define sdl_example_LFPOnline_h

	#if defined(LFP_ONLINE_LIB_EXPORT) && defined(_WIN32) // inside DLL
		#   define LFPONLINEAPI   __declspec(dllexport)
	#else // outside DLL
		#   define LFPONLINEAPI   __declspec(dllimport)
	#endif  // LFP_ONLINE_LIB_EXPORT

#endif // sdl_example_LFPOnline_h
