/*
 * Config.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: igor
 */

#include "Config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

const unsigned int NPROC = 25;

const char *Config::known_processors_ar[] = {"Autocorrelogram", "CluReaderClustering", "FetFileReader",
		"FrequencyPowerBand", "GMMClustering", "KDClustering", "PackageExtractor", "PlaceField", "PCAExtraction",
		"PositionDisplay", "PositionReconstruction", "SDLControlInputMeta", "SDLPCADisplay",
		"SDLSignalDisplay", "SDLWaveshapeDisplay", "SlowDown", "SpeedEstimation", "SpikeAlignment",
		"SpikeDetector", "SwReader", "TransProbEstimation", "UnitTesting", "WaveshapeReconstruction",
		"WhlFileReader", "LPTTrigger"};

const std::vector<std::string> Config::known_processors_(Config::known_processors_ar, Config::known_processors_ar + NPROC);

void Config::read_processors(std::ifstream& fconf) {
	int numproc;
	fconf >> numproc;
	std::cout << numproc << " processors to be used in the pipeline\n";

	std::string proc_name;
	for (int p = 0; p < numproc; ++p) {
		fconf >> proc_name;
		if (std::find(known_processors_.begin(), known_processors_.end(), proc_name) == known_processors_.end()){
			std::cout << "ERROR: Unknown processor: " << proc_name << ". Terminating...\n";
			exit(1);
		}

		processors_list_.push_back(proc_name);
	}
}

Config::Config(std::string path) {
	std::ifstream fconf(path);
	// parse out pairs <param_name>, <string value> from lines
	// <param name> <param_value>
	// with commend lines starting with '//'
	std::string line;

	std::cout << "Read config file: " << path << "\n";

	while(std::getline(fconf, line)){
		if (line[0] == '/' || line.length() == 0)
			continue;

		if (line == std::string("pipeline")){
			read_processors(fconf);
			continue;
		}

		if (line == std::string("tetrodes")){
			int ntetr, tetr;
			fconf >> ntetr;
			for (int t = 0; t < ntetr; ++t) {
				fconf >> tetr;
				tetrodes.push_back(tetr);
			}
			continue;
		}

		std::istringstream ssline(line);

		std::string key;
		if (std::getline(ssline, key, '=')){
			std::string value;
			if (std::getline(ssline, value)){
				params_[key] = value;
				std::cout << " " << key << " = " << value << "\n";

				if (key == "lfpdisp.channels.number"){
					// read lfp channel numbers to display
					int nchan = atoi(value.c_str());
					lfp_disp_channels_ = new unsigned int[nchan];
					int chan;
					for (int ch = 0; ch < nchan; ++ch) {
						fconf >> chan;
						lfp_disp_channels_[ch] = chan;
					}
				}

			}
			else{
				std::cout << "WARNING: unreadable config entry: " << line << "\n";
			}
		}
		else{
			std::cout << "WARNING: unreadable config entry: " << line << "\n";
		}
	}

	fconf.close();
}

bool Config::check_parameter(std::string name, bool exit_on_fail){
	if (params_.find(name) == params_.end()){
		std::cout << (exit_on_fail ? "ERROR" : "WARNING") << ": no parameter named " << name << "\n";
		if (exit_on_fail){
			exit(1);
		}else{
			return false;
		}
	}
	return true;
}

int Config::getInt(std::string name) {
	check_parameter(name);
	requested_params_.insert(name);

	return atoi(params_[name].c_str());
}

float Config::getFloat(std::string name) {
	check_parameter(name);
	requested_params_.insert(name);

	return atof(params_[name].c_str());
}

bool Config::getBool(std::string name) {
	check_parameter(name);
	requested_params_.insert(name);

	return (bool)atoi(params_[name].c_str());
}

std::string Config::getString(std::string name) {
	check_parameter(name);
	requested_params_.insert(name);

	return params_[name];
}

void Config::checkUnused() {
	for(std::map<std::string, std::string>::iterator iter = params_.begin(); iter != params_.end(); ++iter){
		if (requested_params_.find(iter->first) == requested_params_.end()){
			std::cout << "WARNING: param " << iter->first << " read but not requested\n";
		}
	}
}

int Config::getInt(std::string name, const int def_val) {
	if (check_parameter(name, false))
		return getInt(name);
	else{
		std::cout << "WARNING: using default value " << def_val << " for parameter " << name << "\n";
		return def_val;
	}
}

float Config::getFloat(std::string name, const float def_val) {
	if (check_parameter(name, false))
			return getFloat(name);
	else{
		std::cout << "WARNING: using default value " << def_val << " for parameter " << name << "\n";
		return def_val;
	}
}

bool Config::getBool(std::string name, bool def_val) {
	if (check_parameter(name, false))
		return getBool(name);
	else{
		std::cout << "WARNING: using default value " << def_val << " for parameter " << name << "\n";
		return def_val;
	}
}

std::string Config::getString(std::string name, std::string def_val) {
	if (check_parameter(name, false))
		return getString(name);
	else{
		std::cout << "WARNING: using default value " << def_val << " for parameter " << name << "\n";
		return def_val;
	}
}

Config::~Config() {
	// TODO Auto-generated destructor stub
}

