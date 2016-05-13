/*
 * Copyright (C) 2010-2011 Daniel Richter <danielrichter2007@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef SCRIPTSOURCEMAP_H_
#define SCRIPTSOURCEMAP_H_

#include <map>
#include <string>
#include <dirent.h>

#include "../lib/CsvProcessor.hpp"
#include "../lib/Trait/LoggerAware.hpp"
#include "Env.hpp"

class Model_ScriptSourceMap :
	public std::map<std::string, std::string>,
	public Trait_LoggerAware,
	public Model_Env_Connection
{
	std::string _getFilePath() {
		return this->env->cfg_dir + "/.script_sources.txt";
	}

	bool _fileExists;
	std::list<std::string> _newSources;
public:
	Model_ScriptSourceMap() : _fileExists(false)
	{
	}

	void load() {
		this->clear();
		this->_fileExists = false;
		this->_newSources.clear();
	
		FILE* file = fopen(this->_getFilePath().c_str(), "r");
		if (file) {
			this->_fileExists = true;
			CsvReader csv(file);
			std::map<std::string, std::string> dataRow;
			while ((dataRow = csv.read()).size()) {
				(*this)[this->env->cfg_dir + "/" + dataRow["default_name"]] = this->env->cfg_dir + "/" + dataRow["current_name"];
			}
			fclose(file);
		}
	}

	void registerMove(std::string const& sourceName, std::string const& destinationName) {
		std::string originalSourceName = this->getSourceName(sourceName);
		if (originalSourceName != "") { // update existing script entry
			(*this)[originalSourceName] = destinationName;
		} else {
			(*this)[sourceName] = destinationName;
		}
	}

	void addScript(std::string const& sourceName) {
		if (this->has(sourceName)) {
			this->_newSources.push_back(sourceName);
		} else {
			(*this)[sourceName] = sourceName;
		}
	}

	void save() {
		FILE* file = fopen(this->_getFilePath().c_str(), "w");
		assert(file != NULL);
		CsvWriter csv(file);
		for (std::map<std::string, std::string>::iterator iter = this->begin(); iter != this->end(); iter++) {
			if (iter->first == iter->second) {
				continue;
			}
			if (iter->first.substr(0, this->env->cfg_dir.length()) != this->env->cfg_dir
			 || iter->second.substr(0, this->env->cfg_dir.length()) != this->env->cfg_dir) {
				this->log("invalid script prefix found: script wont be added to source map", Logger::ERROR);
				continue; // ignore, if path doesn't start with cfg_dir
			}
			std::string defaultName = iter->first.substr(this->env->cfg_dir.length() + 1);
			std::string currentName = iter->second.substr(this->env->cfg_dir.length() + 1);
			std::map<std::string, std::string> dataRow;
			dataRow["default_name"] = defaultName;
			dataRow["current_name"] = currentName;
			csv.write(dataRow);
		}
		fclose(file);
	}

	bool has(std::string const& sourceName) {
		return this->find(sourceName) != this->end();
	}

	std::string getSourceName(std::string const& destinationName) {
		for (std::map<std::string, std::string>::iterator iter = this->begin(); iter != this->end(); iter++) {
			if (iter->second == destinationName) {
				return iter->first;
			}
		}
		return "";
	}

	bool fileExists() {
		return this->_fileExists;
	}

	std::list<std::string> getUpdates() const {
		return this->_newSources;
	}

	void deleteUpdates() {
		this->_newSources.clear();
	}

};


#endif /* SCRIPTSOURCEMAP_H_ */
