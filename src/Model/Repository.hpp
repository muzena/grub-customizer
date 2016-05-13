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

#ifndef GRUB_CUSTOMIZER_REPOSITORY_INCLUDED
#define GRUB_CUSTOMIZER_REPOSITORY_INCLUDED
#include <sys/stat.h>
#include <dirent.h>
#include <map>
#include <memory>
#include "../lib/Trait/LoggerAware.hpp"
#include "../lib/ArrayStructure.hpp"
#include "../lib/Helper.hpp"
#include "ProxyScriptData.hpp"
#include "PscriptnameTranslator.hpp"
#include "Script.hpp"

class Model_Repository : public std::list<std::shared_ptr<Model_Script>>, public Trait_LoggerAware
{
	public: std::list<std::shared_ptr<Model_Script>> trash;

	public: void load(std::string const& directory, bool is_proxifiedScript_dir)
	{
		DIR* dir = opendir(directory.c_str());
		if (dir){
			struct dirent *entry;
			struct stat fileProperties;
			while ((entry = readdir(dir))) {
				stat((directory+"/"+entry->d_name).c_str(), &fileProperties);
				if ((fileProperties.st_mode & S_IFMT) != S_IFDIR){ //ignore directories
					bool scriptAdded = false;
					if (!is_proxifiedScript_dir && !Model_ProxyScriptData::is_proxyscript(directory+"/"+entry->d_name) && std::string(entry->d_name).length() >= 4 && entry->d_name[0] >= '0' && entry->d_name[0] <= '9' && entry->d_name[1] >= '0' && entry->d_name[1] <= '9' && entry->d_name[2] == '_'){
						this->push_back(std::make_shared<Model_Script>(std::string(entry->d_name).substr(3), directory+"/"+entry->d_name));
						scriptAdded = true;
					} else if (is_proxifiedScript_dir) {
						this->push_back(std::make_shared<Model_Script>(Model_PscriptnameTranslator::decode(entry->d_name), directory+"/"+entry->d_name));
						scriptAdded = true;
					}
					if (scriptAdded && this->hasLogger()) {
						this->back()->setLogger(this->getLogger());
					}
				}
			}
			closedir(dir);
		}
	}

	public: std::shared_ptr<Model_Script> getScriptByFilename(std::string const& fileName, bool createScriptIfNotFound = false)
	{
		for (auto script : *this) {
			if (script->fileName == fileName)
				return script;
		}
		if (createScriptIfNotFound){
			this->push_back(std::make_shared<Model_Script>("noname", fileName));
			auto newScript = this->back();
	
			if (this->hasLogger()) {
				this->back()->setLogger(this->getLogger());
			}

			return newScript;
		}
		return nullptr;
	}

	public: std::shared_ptr<Model_Script> getScriptByName(std::string const& name)
	{
		for (auto script : *this) {
			if (script->name == name) {
				return script;
			}
		}
		return nullptr;
	}

	public: std::shared_ptr<Model_Script> getScriptByEntry(std::shared_ptr<Model_Entry> entry)
	{
		for (auto script : *this) {
			if (script->hasEntry(entry)) {
				return script;
			}
		}
		return nullptr;
	}

	public: std::shared_ptr<Model_Script const> getScriptByEntry(std::shared_ptr<Model_Entry> entry) const
	{
		for (auto script : *this) {
			if (script->hasEntry(entry)) {
				return script;
			}
		}
		return nullptr;
	}

	public: std::shared_ptr<Model_Script> getCustomScript()
	{
		for (auto script : *this) {
			if (script->isCustomScript) {
				return script;
			}
		}
		return nullptr;
	}

	public: std::shared_ptr<Model_Script> getNthScript(int pos)
	{
		int i = 0;
		for (auto script : *this) {
			if (i == pos) {
				return script;
			}
			i++;
		}
		return nullptr;
	}

	public: void deleteAllEntries(bool preserveModifiedScripts = true)
	{
		for (auto script : *this) {
			if (preserveModifiedScripts && script->isModified()) {
				continue;
			}
			script->entries().clear();
		}
	}

	public: std::shared_ptr<Model_Script> createScript(
		std::string const& name,
		std::string const& fileName,
		std::string const& content
	) {
		Helper::assert_filepath_empty(fileName, __FILE__, __LINE__);
		FILE* script = fopen(fileName.c_str(), "w");
		if (script) {
			fputs(content.c_str(), script);
			fclose(script);

			this->push_back(std::make_shared<Model_Script>(name, fileName));
			return this->back();
		}
		return nullptr;
	}

	public: void createScript(std::shared_ptr<Model_Script> script, std::string const& content)
	{
		Helper::assert_filepath_empty(script->fileName, __FILE__, __LINE__);
		FILE* scriptFile = fopen(script->fileName.c_str(), "w");
		if (scriptFile) {
			fputs(content.c_str(), scriptFile);
			fclose(scriptFile);
		} else {
			throw FileSaveException("cannot open file for saving: " + script->fileName, __FILE__, __LINE__);
		}
	}

	// create existing script (in scriptlist) on file system
	public: std::map<std::string, std::shared_ptr<Model_Script>> getScriptPathMap()
	{
		std::map<std::string, std::shared_ptr<Model_Script>> map;
		for (auto script : *this) {
			map[script->fileName] = script;
		}
		for (auto trashScript : this->trash){
			map[trashScript->fileName] = trashScript;
		}
		return map;
	}

	public: void removeScript(std::shared_ptr<Model_Script> script)
	{
		for (auto scriptIter = this->begin(); scriptIter != this->end(); scriptIter++) {
			if (*scriptIter == script) {
				this->trash.push_back(*scriptIter);
				this->erase(scriptIter);
				return;
			}
		}
	}

	public: void clearTrash()
	{
		for (auto script : this->trash) {
			script->deleteFile();
		}
		this->trash.clear();
	}

	public: operator ArrayStructure() const
	{
		ArrayStructure result;
		result["(items)"].isArray = true;
		int i = 0;
		for (auto script : *this) {
			result["(items)"][i] = ArrayStructure(*script);
			i++;
		}
		return result;
	}

};

#endif
