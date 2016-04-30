#include "Utils.h"
#include <sstream>

const char * const Utils::NUMBERS[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61", "62", "63"};

std::string Utils::Converter::int2str(int a){
    if (a == 0)
        return "0";
    
    std::string s = "";
    
    while(a > 0){
        s = std::string(Utils::NUMBERS[a % 10]) + s;
        a = a / 10;
    }
    
    return s;
}

 std::string Utils::Converter::Combine(std::string s, int a){
	 return s + int2str(a);
 }

 std::string Utils::Converter::Combine(const char* s, int a){
	 return std::string(s) + int2str(a);
 }

 std::string Utils::Converter::Combine(const char* s, double a){
	 std::stringstream ss("");
	 ss << s;
	 ss.precision(2);
	 ss << a;
 	 return ss.str();
  }

#ifdef WIN32
char* Utils::Converter::WstringToCstring(wchar_t *wstring){
	size_t convertedChars = 0;
	size_t origsize = wcslen(wstring) + 1;
	const size_t newsize = origsize*2;
	char *nstring = new char[newsize];
	wcstombs_s(&convertedChars, nstring, newsize, wstring, _TRUNCATE);
	return nstring;
}
#endif

std::vector<int> Utils::Math::GetRange(const unsigned int& from, const unsigned int& to){
    std::vector<int> range;
    for (unsigned int i = 0; i < to-from+1; ++i) {
        range.push_back(from + i);
    }
    return range;
}

std::vector<int> Utils::Math::MergeRanges(const std::vector<int>& a1, const std::vector<int>& a2){
    std::vector<int> merged;
    merged = a1;
    
    for (size_t i = 0; i < a2.size(); ++i) {
        merged.push_back(a2[i]);
    }
    
    return merged;
}

bool Utils::Math::Isnan(float f){
#ifdef _WIN32
	return std::isnan<float>(f);
#else
	return isnan(f);
#endif
}

void Utils::Output::printIntArray(int *array, const unsigned int num_el, std::ostream stream){
    for (unsigned int e = 0; e < num_el; ++e) {
    	stream << array[e] << " ";
    }
    stream << "\n";
}

void Utils::FS::CheckFileExistsWithError(const std::string& filename, Utils::Logger* logger){
	if (!FileExists(filename)){
		logger->Log(std::string("ERROR: File ") + filename + " doesn't exist or is unavailable");
		exit(82346);
	}

}

Utils::NewtonSolver::NewtonSolver(const double& target_f,
		const unsigned int& update_frequency, const double& alpha,
		const double& init_x)
: target_f_(target_f)
, update_frequency_(update_frequency)
, alpha_(alpha)
, current_x_(init_x)
, last_update_(0){
}

double Utils::NewtonSolver::Update(const unsigned int& time,
		const double& f_value_) {
	if (time > last_update_ + update_frequency_){
		current_x_ += (target_f_ - f_value_) * alpha_;
		last_update_ = time;
	}

	return current_x_;
}

bool Utils::NewtonSolver::NeedUpdate(const unsigned int& time) {
	return time > last_update_ + update_frequency_;
}
