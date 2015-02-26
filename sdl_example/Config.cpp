/*
 * Config.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: igor
 */

#include "Config.h"
#include "LFPBuffer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

void Config::read_processors(std::ifstream& fconf) {
	std::string log_prefix = params_.find("out.path.base") == params_.end() ? "" : params_["out.path.base"];
	log_.open(log_prefix + "lfponline_config_log.txt");

	int numproc;
	fconf >> numproc;
	log_string_stream_ << numproc << " processors to be used in the pipeline\n";
	Log();

	std::string proc_name;
	for (int p = 0; p < numproc; ++p) {
		fconf >> proc_name;
		if (std::find(known_processors_.begin(), known_processors_.end(), proc_name) == known_processors_.end() &&
				(proc_name[0]!='/' || proc_name[1]!='/' ||
						std::find(known_processors_.begin(), known_processors_.end(), proc_name.substr(2, proc_name.length() - 2)) == known_processors_.end())){
			log_string_stream_ << "ERROR: Unknown processor: " << proc_name << ". Terminating...\n";
			Log();
			log_.close();
			exit(LFPONLINE_ERROR_UNKNOWN_PROCESSOR);
		}

		if (!(proc_name[0]=='/' && proc_name[1] == '/'))
			processors_list_.push_back(proc_name);
	}
	log_.flush();
}



Config::Config(std::string path) {
	config_path_ = path;

	std::ifstream fconf(path);
	// parse out pairs <param_name>, <string value> from lines
	// <param name> <param_value>
	// with commend lines starting with '//'
	std::string line;

	known_processors_.push_back("PackageExtractor");
	known_processors_.push_back("SpikeDetector");
	known_processors_.push_back("Autocorrelogram");
	known_processors_.push_back("CluReaderClustering");
	known_processors_.push_back("FetFileReader");
	known_processors_.push_back("FrequencyPowerBand");
	known_processors_.push_back("GMMClustering");
	known_processors_.push_back("KDClustering");
	known_processors_.push_back("PlaceField");
	known_processors_.push_back("PCAExtraction");
	known_processors_.push_back("PositionDisplay");
	known_processors_.push_back("PositionReconstruction");
	known_processors_.push_back("SDLControlInputMeta");
	known_processors_.push_back("SDLPCADisplay");
	known_processors_.push_back("SDLSignalDisplay");
	known_processors_.push_back("SDLWaveshapeDisplay");
	known_processors_.push_back("SlowDown");
	known_processors_.push_back("SpeedEstimation");
	known_processors_.push_back("SpikeAlignment");
	known_processors_.push_back("SwReader");
	known_processors_.push_back("TransProbEstimation");
	known_processors_.push_back("UnitTesting");
	known_processors_.push_back("WaveshapeReconstruction");
	known_processors_.push_back("WhlFileReader");
	known_processors_.push_back("LPTTrigger");
	known_processors_.push_back("FetFileWriter");
	known_processors_.push_back("BinFileReader");


	//for (size_t i = 0; i < NPROC; i++)
	//{
	//	std::string proc(known_processors_ar[i]);
	//	known_processors_.push_back(proc);	
	//}

	log_string_stream_ << "Read config file: " << path << "\n";
	Log();

	while(std::getline(fconf, line)){
		if (line.length() == 0 || line[0] == '/')
			continue;

		if (line == std::string("pipeline")){
			read_processors(fconf);
			continue;
		}

		if (line == std::string("tetrodes")){
			ReadList(fconf, tetrodes);
			continue;
		}

		if (line == std::string("synchrony")){
			ReadList<unsigned int>(fconf, synchrony_tetrodes_);
			continue;
		}

		if (line == std::string("lfpdisp.channels")){
			ReadList<unsigned int>(fconf, lfp_disp_channels_);
		}

		std::istringstream ssline(line);

		std::string key;
		if (std::getline(ssline, key, '=')){
			std::string value;
			if (std::getline(ssline, value)){
				params_[key] = value;
				log_string_stream_ << " " << key << " = " << value << "\n";
				Log();

				if (key == "spike.reader.files.number"){
					int nfiles = atoi(value.c_str());
					for (int i=0; i < nfiles; ++i){
						std::getline(fconf, line);
						if (line[0] != '/' || line[1] != '\''){
							spike_files_.push_back(line);
						}
					}
				}
			}
			else{
				log_string_stream_ << "WARNING: unreadable config entry: " << line << "\n";
				Log();
			}
		}
		else{
			log_string_stream_ << "WARNING: unreadable config entry: " << line << "\n";
			Log();
		}
	}

	fconf.close();
}

bool Config::check_parameter(std::string name, bool exit_on_fail){
	if (params_.find(name) == params_.end()){
		log_string_stream_ << (exit_on_fail ? "ERROR" : "WARNING") << ": no parameter named " << name << "\n";
		if (exit_on_fail){
			log_string_stream_ << "ERROR: no parameter named " << name << "\n";
			Log();
			log_.close();
			exit(LFPONLINE_ERROR_MISSING_PARAMETER);
		}else{
			Log();
			return false;
		}
	}
	return true;
}

int Config::getInt(std::string name) {
	check_parameter(name);
	//requested_params_.insert(name);

	return atoi(params_[name].c_str());
}

float Config::getFloat(std::string name) {
	check_parameter(name);
	//requested_params_.insert(name);

	return atof(params_[name].c_str());
}

bool Config::getBool(std::string name) {
	check_parameter(name);
	//requested_params_.insert(name);

	return (bool)atoi(params_[name].c_str());
}

std::string Config::getString(std::string name) {
	check_parameter(name);
	//requested_params_.insert(name);

	return params_[name];
}

void Config::checkUnused() {
	for(std::map<std::string, std::string>::iterator iter = params_.begin(); iter != params_.end(); ++iter){
		//if (requested_params_.find(iter->first) == requested_params_.end()){
		log_string_stream_ << "WARNING: param " << iter->first << " read but not requested\n";
		Log();
		//}
	}
}

int Config::getInt(std::string name, const int def_val) {
	if (check_parameter(name, false))
		return getInt(name);
	else{
		log_string_stream_ << "WARNING: using default value " << def_val << " for parameter " << name << "\n";
		Log();
		return def_val;
	}
}

float Config::getFloat(std::string name, const float def_val) {
	if (check_parameter(name, false))
			return getFloat(name);
	else{
		log_string_stream_ << "WARNING: using default value " << def_val << " for parameter " << name << "\n";
		Log();
		return def_val;
	}
}

bool Config::getBool(std::string name, bool def_val) {
	if (check_parameter(name, false))
		return getBool(name);
	else{
		log_string_stream_ << "WARNING: using default value " << def_val << " for parameter " << name << "\n";
		Log();
		return def_val;
	}
}

std::string Config::getString(std::string name, std::string def_val) {
	if (check_parameter(name, false))
		return getString(name);
	else{
		log_string_stream_ << "WARNING: using default value " << def_val << " for parameter " << name << "\n";
		Log();
		return def_val;
	}
}

std::string Config::getOutPath(std::string outname) {
	std::string outval = getString(outname);
	bool append = getBool("out.path.append", false);

	if (append){
		outval = getString("out.path.base") + outval;
	}

	Utils::FS::CreateDirectories(outval);
	return outval;
}

std::string Config::getOutPath(std::string outname,
		std::string default_append) {
	bool append = getBool("out.path.append");
	if (!check_parameter(outname, false)){
		if (append){
			return getString("out.path.base") + default_append;
		}
		else{
			log_ << "ERROR: default out path is allowed only if append is enabled, not path for " << outname << "\n";
			exit(LFPONLINE_ERROR_DEFAULT_OUTPATH_NOT_ALLOWED);
		}
	}
	else{
		return getOutPath(outname);
	}
}

Config::~Config() {
	log_.close();
}

void Config::Log() {
	log_ << log_string_stream_.str();
	std::cout << log_string_stream_.str();
	log_string_stream_.str(std::string());
}

std::string Config::getAllParamsText() {
	std::stringstream ss;

	for (std::map<std::string, std::string>::iterator it=params_.begin(); it!=params_.end(); ++it){
		ss << it->first << "=" << it->second << "\n";
	}

	return ss.str();
}

template<class T>
void Config::ReadList(std::ifstream& file, std::vector<T>& list) {
	int nentry;
	T entry;
	file >> nentry;
	for (int t = 0; t < nentry; ++t) {
		file >> entry;
		list.push_back(entry);
	}
}
