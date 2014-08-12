/*
 * Config.h
 *
 *  Created on: Jul 28, 2014
 *      Author: igor
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <set>
#include <fstream>
#include <vector>

class Config {

std::map<std::string, std::string> params_;
std::set<std::string> requested_params_;

static const char *known_processors_ar[];

bool check_parameter(std::string name, bool exit_on_fail = true);
void read_processors(std::ifstream& fconf);

public:
	static const std::vector<std::string> known_processors_;

	std::vector<std::string> processors_list_;

	// which whl/clu/res etc files to process
	std::vector<int> tetrodes;

	// channels to be displayed in the SDLSignalDisplay
	int unsigned  *lfp_disp_channels_ = NULL;

	Config(std::string path);

	int getInt(std::string name);
	int getInt(std::string name, const int def_val);
	float getFloat(std::string name);
	float getFloat(std::string name, const float def_val);
	bool getBool(std::string name);
	bool getBool(std::string name, bool def_val);
	std::string getString(std::string name);
	std::string getString(std::string name, std::string def_val);

	void checkUnused();

	virtual ~Config();
};

#endif /* CONFIG_H_ */
