#include "Utils.h"

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

void Utils::Output::printIntArray(int *array, const unsigned int num_el, std::ostream stream){
    for (unsigned int e = 0; e < num_el; ++e) {
    	stream << array[e] << " ";
    }
    stream << "\n";
}
