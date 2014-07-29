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

		std::istringstream ssline(line);

		std::string key;
		if (std::getline(ssline, key, '=')){
			std::string value;
			if (std::getline(ssline, value)){
				params_[key] = value;
				std::cout << " " << key << " = " << value << "\n";
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

bool Config::check_parameter(std::string name){
	if (params_.find(name) == params_.end()){
		std::cout << "ERROR: no parameter named " << name;
		exit(1);
	}
	return true;
}

int Config::getInt(std::string name) {
	check_parameter(name);

	return atoi(params_[name].c_str());
}

float Config::getFloat(std::string name) {
	check_parameter(name);

	return atof(params_[name].c_str());
}

bool Config::getBool(std::string name) {
	check_parameter(name);

	return (bool)atoi(params_[name].c_str());
}

std::string Config::getString(std::string name) {
	check_parameter(name);

	return params_[name];
}

Config::~Config() {
	// TODO Auto-generated destructor stub
}

