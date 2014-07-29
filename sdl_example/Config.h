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

class Config {

std::map<std::string, std::string> params_;
std::set<std::string> requested_params_;

bool check_parameter(std::string name);

public:
	Config(std::string path);

	int getInt(std::string name);
	float getFloat(std::string name);
	bool getBool(std::string name);
	std::string getString(std::string name);

	void checkUnused();

	virtual ~Config();
};

#endif /* CONFIG_H_ */
