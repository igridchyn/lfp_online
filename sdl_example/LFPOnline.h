#ifndef sdl_example_LFPOnline_h
#define sdl_example_LFPOnline_h

	#ifndef LFPONLINEAPI
		#ifdef _WIN32
			#if defined(LFP_ONLINE_LIB_EXPORT) // inside DLL
				#   define LFPONLINEAPI   __declspec(dllexport)
			#else // outside DLL
				#   define LFPONLINEAPI   __declspec(dllimport)
			#endif  // LFP_ONLINE_LIB_EXPORT

			#pragma warning( disable: 4251 )
		#else
			#define LFPONLINEAPI
		#endif
	#endif

#endif // sdl_example_LFPOnline_h
