#ifndef Utils_h
#define Utils_h

#include <iostream>
#include <vector>
#include <math.h>
#include <boost/filesystem.hpp>
#include "LFPOnline.h"
#ifdef _WIN32
	#define M_PI 3.14159265358979323846
#endif

class LFPONLINEAPI Utils{
public:
    static const char* const NUMBERS[];
    
    class Logger{
    public:
    	virtual void Log() = 0;
    	virtual void Log(std::string message) = 0;
    	virtual void Log(std::string message, int num) = 0;
    	virtual void Log(std::string message, unsigned int num) = 0;
    #ifndef _WIN32
    	virtual void Log(std::string message, size_t num) = 0;
    #endif
    	virtual void Log(std::string message, double num) = 0;
    };

	class LFPONLINEAPI Converter{
    public:
        static std::string int2str(int a);
        static std::string Combine(std::string s, int a);
        static std::string Combine(const char* s, int a);
        static std::string Combine(const char* s, double a);
#ifdef WIN32
        static char* WstringToCstring(wchar_t *wstring);
#endif
    };
    
    class Math{
    public:
        inline static double Gauss2D(double sigma, double x, double y) { return 1/(2 * M_PI * sqrt(sigma)) * exp(-0.5 * (pow(x, 2) + pow(y, 2)) / (sigma * sigma)); };
        static std::vector<int> GetRange(const unsigned int& from, const unsigned int& to);
        static std::vector<int> MergeRanges(const std::vector<int>& a1, const std::vector<int>& a2);
    };
    
    class Output{
    public:
        static void printIntArray(int *array, const unsigned int num_el, std::ostream stream);
    };
    
    class FS{
    public:
        static bool FileExists(const std::string& filename)
        {
            return boost::filesystem::exists(filename);
        }
        
        // create directories at all levels up to the file specified in the argument
        static bool CreateDirectories(const std::string file_path){
            std::string  path_dir = file_path.substr(0, file_path.find_last_of('/'));
            
            if(!boost::filesystem::exists(path_dir)){
                boost::filesystem::create_directories(path_dir);
            }
            
            return true;
        }
    };
};

#endif
