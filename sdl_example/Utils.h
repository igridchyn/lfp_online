#include <iostream>
#include <vector>
#include <math.h>
#include <boost/filesystem.hpp>
#ifdef _WIN32
	#define M_PI 3.14159265358979323846
#endif

class Utils{
public:
    static const char* const NUMBERS[];
    
    class Converter{
    public:
        static std::string int2str(int a);
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
