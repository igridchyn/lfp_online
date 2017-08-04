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
#include "time.h"

#ifdef _WIN32
	#define DATEFORMAT "%Y-%m-%d_%H-%M-%S"
#else
	#define DATEFORMAT "%F_%T"
#endif

void Config::read_processors(std::ifstream& fconf) {
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



Config::Config(std::string path, unsigned int nparams, char **params, std::map<std::string, std::string> *initMap) {
	// generate timestamp
	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (buffer, 80, DATEFORMAT, timeinfo);
	buffer[13] = '-';
	buffer[16] = '-';
	timestamp_ = buffer;

	if (initMap == nullptr){
		params_["timestamp"] = timestamp_;
	} else {
		params_.insert(initMap->begin(), initMap->end());
	}

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
	known_processors_.push_back("FiringRateEstimator");
	known_processors_.push_back("BinaryPopulationClassifier");
	known_processors_.push_back("ParallelPipeline");

	for (unsigned int i = 0; i < nparams; ++i){
		std::string paramline(params[i]);
		log_string_stream_ << "OVERRIDE: " << paramline << "\n";
		Log();
		parse_line(fconf, paramline);
	}

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

		// this is now set externally after reading tetrodes config
//		if (line == std::string("tetrodes")){
//			ReadList(fconf, tetrodes);
//			continue;
//		}

		if (line == std::string("synchrony")){
			ReadList<unsigned int>(fconf, synchrony_tetrodes_);
			continue;
		}

		if (line == std::string("lfpdisplay.channels")){
			ReadList<unsigned int>(fconf, lfp_disp_channels_);
			continue;
		}

		if (line == std::string("discriminators")){
			ReadList<unsigned int>(fconf, discriminators_);
			continue;
		}
		if (line == std::string("parallel")){
			ReadList<unsigned int>(fconf, parallel_);
			continue;
		}
		if (line == std::string("pf.sessions")){
			ReadList<unsigned int>(fconf, pf_sessions_);
			continue;
		}
		if (line == std::string("kd.tetrodes")){
			ReadList<unsigned int>(fconf, kd_tetrodes_);
			continue;
		}
		if (line == std::string("#include")){
			std::string confPath;
			std::getline(fconf, confPath);

			confPath = evaluate_variables("#include", confPath);

			Utils::FS::CheckFileExistsWithError(confPath, (Utils::Logger*)this);

			Config includedConf(confPath, 0, nullptr, &params_);
			params_.insert(includedConf.params_.begin(), includedConf.params_.end());
			log_string_stream_ << "Include params from file: " << confPath << "\n";
			log_string_stream_ << includedConf.params_.size() << " params included\n";
			Log();

			if (includedConf.synchrony_tetrodes_.size() > 0){
				if (synchrony_tetrodes_.size() > 0){
					Log("WARNING: override list content");
				}
				synchrony_tetrodes_ = includedConf.synchrony_tetrodes_;
			}
			if (includedConf.lfp_disp_channels_.size() > 0){
				if (lfp_disp_channels_.size() > 0){
					Log("WARNING: override list content");
				}
				lfp_disp_channels_ = includedConf.lfp_disp_channels_;
			}
			if (includedConf.discriminators_.size() > 0){
				if (discriminators_.size() > 0){
					Log("WARNING: override list content");
				}
				discriminators_ = includedConf.discriminators_;
			}
			if (includedConf.pf_sessions_.size() > 0){
				if (pf_sessions_.size() > 0){
					Log("WARNING: override list content");
				}
				pf_sessions_ = includedConf.pf_sessions_;
			}
			if ( includedConf.spike_files_.size() > 0){
				if (spike_files_.size() > 0){
					Log("WARNING: override list content");
				}
				spike_files_ = includedConf.spike_files_;
			}

			continue;
		}

		parse_line(fconf, line);
	}

	fconf.close();
}

bool Config::check_parameter(std::string name, bool exit_on_fail){
	if (params_.find(name) == params_.end()){
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

	return (float)atof(params_[name].c_str());
}

bool Config::getBool(std::string name) {
	check_parameter(name);
	//requested_params_.insert(name);

	return atoi(params_[name].c_str()) != 0;
}

std::string Config::getString(std::string name) {
	check_parameter(name);
	//requested_params_.insert(name);

	return params_[name];
}

void Config::checkUnused() {
	for(auto iter = params_.begin(); iter != params_.end(); ++iter){
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

void Config::Log(bool nocout) {
	log_ << log_string_stream_.str();
	if (!nocout)
		std::cout << log_string_stream_.str();
	log_string_stream_.str(std::string());
}

std::string Config::getAllParamsText() {
	std::stringstream ss;

	for (auto it=params_.begin(); it!=params_.end(); ++it){
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

std::string Config::evaluate_variables(std::string key, std::string value) {
	while (value.find('$') != std::string::npos && value.find('{') != std::string::npos){
		int pstart = value.find('$');

		if(value.find('}') == std::string::npos){
			log_string_stream_ << "ERROR: Cannot find closing of variable name for parameter " << key;
			Log();
			exit(1);
		}

		std::string parname = value.substr(pstart + 2, value.find('}') -pstart - 2);
		if (params_.find(parname) == params_.end()){
			log_string_stream_ << "ERROR: variable " << parname << " is not defined before parameter " << key;
			Log();
			exit(1);
		}

		value = value.substr(0, pstart) + params_[parname] + value.substr(value.find('}') + 1, value.size() - value.find('}') - 1);
	}

	return value;
}

void Config::parse_line(std::ifstream& fconf, std::string line) {
	std::istringstream ssline(line);

	std::string key;
	if (std::getline(ssline, key, '=')){
		std::string value;
		if (std::getline(ssline, value)){
			// look for variables
			value = evaluate_variables(key, value);

			if (params_.find(key) != params_.end()){
				log_string_stream_ << "WARNING: IGNORE REPEATED ENTRY OF " << key << "\n";
				log_string_stream_ << "  the first provided value was: " << params_[key] << "\n";
				Log();
				return;
			}

			params_[key] = value;
			log_string_stream_ << " " << key << " = " << value << "\n";
			Log(true);

			if (key == "spike.reader.files.number"){
				int nfiles = atoi(value.c_str());
				for (int i=0; i < nfiles; ++i){
					std::getline(fconf, line);
					if (line[0] != '/' || line[1] != '\''){
						spike_files_.push_back(evaluate_variables("spike.reader.files", line));
					}
				}
			}
		}
		else{
			log_string_stream_ << "WARNING: unreadable config entry: " << line << "\n";
			Log();
		}

		if (key == "out.path.base"){
			Utils::FS::CreateDirectories(value);

			Utils::FS::CreateDirectories(value + "logs/");
			log_.open(value + "logs/lfpo_conf_" + timestamp_ + ".log");
			log_ << "Session timestamp: " << timestamp_ << "\n";
		}
	}
	else{
		log_string_stream_ << "WARNING: unreadable config entry: " << line << "\n";
		Log();
	}
}

void Config::setTetrodes(const unsigned int& tetrodes_count){
	if (tetrodes.size() > 0){
		log_string_stream_ << "WARNING: tetrodes list not empty. Overriding old entries.\n";
		Log();
	}
	for (unsigned int t=0; t < tetrodes_count; ++t){
		tetrodes.push_back(t);
	}
	log_string_stream_ << "Added number of tetrodes: " << tetrodes_count << "\n";
	Log();
}


template <class T>
void printVector(std::vector<T> vec, std::string vec_name, std::ofstream & stream, bool newline){
	stream << "ARRAY " << vec_name << ":\n";
	std::string endsym = newline ? "\n" : " ";
	for (unsigned int i=0; i < vec.size(); ++i){
		stream << vec[i] << endsym;
	}
	stream << "\n";
}

void Config::dump(std::ofstream & out){
	// dump all lists and params - was in separate file before
	out << "=== CONFIG DUMP START ===\n";
	for(auto iterator = params_.begin(); iterator != params_.end(); iterator++) {
		out << iterator->first << "=" << iterator->second << "\n";
	}

	printVector<unsigned int>(pf_sessions_, "PF sessions", out, true);
	printVector<unsigned int>(synchrony_tetrodes_, "Synchrony tetrodes", out, false);
	printVector<unsigned int>(kd_tetrodes_, "KD tetrodes", out, false);
	printVector<std::string>(spike_files_, "Spike files", out, true);
	printVector<int>(tetrodes, "Tetrodes", out, false);

	out << "CONFIG PATH = " << config_path_ << "\n";
	out << "TIMESTAMP = " << timestamp_ << "\n";

	out << "=== CONFIG DUMP END ===\n";
}
