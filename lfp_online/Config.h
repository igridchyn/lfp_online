/*
 * Config.h
 *
 *  Created on: Jul 28, 2014
 *      Author: Igor Gridchyn
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <set>
#include <fstream>
#include <vector>
#include <sstream>

#include "LFPOnline.h"

class LFPONLINEAPI Config {

std::map<std::string, std::string> params_;
// to check whether params from config file that were intended to be used have ever been requested
// std::set<std::string> requested_params_;

std::ofstream log_;

bool check_parameter(std::string name, bool exit_on_fail = true);
void read_processors(std::ifstream& fconf);

public:
	std::vector<std::string> known_processors_;

	std::vector<std::string> processors_list_;

	// which whl/clu/res etc files to process
	std::vector<int> tetrodes;

	// channels to be displayed in the SDLSignalDisplay
	std::vector<unsigned int> lfp_disp_channels_;

	std::vector<unsigned int> synchrony_tetrodes_;

	std::vector<unsigned int> discriminators_;

	std::vector<unsigned int> pf_sessions_;

	std::vector<unsigned int> pf_groups_;

	std::vector<unsigned int> kd_tetrodes_;

	std::vector<unsigned int> parallel_;

	std::vector<unsigned int> tetrode_nums_;

	std::vector<std::string> spike_files_;

	std::string config_path_;

	std::stringstream log_string_stream_;

	std::string timestamp_;


	template<class T>
	void ReadList(std::ifstream& file, std::vector<T>& list);

	void Log(bool nocout = false);

	Config(std::string path, unsigned int nparams = 0, char **params = nullptr, std::map<std::string, std::string> *initMap = nullptr);

	int getInt(std::string name);
	int getInt(std::string name, const int def_val);
	float getFloat(std::string name);
	float getFloat(std::string name, const float def_val);
	bool getBool(std::string name);
	bool getBool(std::string name, bool def_val);
	std::string getString(std::string name);
	std::string getString(std::string name, std::string def_val);

	std::string getOutPath(std::string outname);
	std::string getOutPath(std::string outname, std::string default_append);

	void checkUnused();

	virtual ~Config();

	std::string getAllParamsText();

	std::string evaluate_variables(std::string key, std::string value);

	void parse_line(std::ifstream& fconf, std::string line);

	void setTetrodes(const unsigned int& tetrodes_count);

	void dump(std::ofstream & out);
};

#endif /* CONFIG_H_ */
