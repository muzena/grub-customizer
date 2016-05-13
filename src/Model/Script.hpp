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

#ifndef GRUB_CUSTOMIZER_SCRIPT_INCLUDED
#define GRUB_CUSTOMIZER_SCRIPT_INCLUDED
#include <string>
#include <list>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include "../Model/EntryPathFollower.hpp"
#include "../lib/Trait/LoggerAware.hpp"
#include "../lib/Helper.hpp"
#include "../config.hpp"
#include "../lib/Exception.hpp"
#include "../lib/ArrayStructure.hpp"
#include "../lib/Type.hpp"
#include "Entry.hpp"

class Model_Script : public Model_EntryPathFollower, public Trait_LoggerAware, public Script {
	public: std::string name, fileName;
	public: bool isCustomScript;
	public: std::shared_ptr<Model_Entry> root;

	public: Model_Script(std::string const& name, std::string const& fileName) :
		name(name),
		fileName(fileName),
		root(std::make_shared<Model_Entry>("DUMMY", "DUMMY", "DUMMY", Model_Entry::SCRIPT_ROOT)),
		isCustomScript(false)
	{
		FILE* script = fopen(fileName.c_str(), "r");
		if (script) {
			Model_Entry_Row row1(script), row2(script);
			if (row1.text == CUSTOM_SCRIPT_SHEBANG && row2.text == CUSTOM_SCRIPT_PREFIX) {
				isCustomScript = true;
			}
			fclose(script);
		}
	}

	public: bool isModified(std::shared_ptr<Model_Entry> parent = nullptr)
	{
		if (!parent) {
			parent = this->root;
		}
		if (parent->isModified) {
			return true;
		}
		for (auto entry : parent->subEntries) {
			if (entry->isModified) {
				return true;
			} else if (entry->type == Model_Entry::SUBMENU) {
				bool modified = this->isModified(entry);
				if (modified) {
					return true;
				}
			}
		}
		return false;
	}

	public: std::list<std::shared_ptr<Model_Entry>>& entries()
	{
		return this->root->subEntries;
	}

	public: std::list<std::shared_ptr<Model_Entry>> const& entries() const
			{
		return this->root->subEntries;
	}

	public: bool isInScriptDir(std::string const& cfg_dir) const
	{
		return this->fileName.substr(cfg_dir.length(), std::string("/proxifiedScripts/").length()) == "/proxifiedScripts/";
	}

	public: std::shared_ptr<Model_Entry> getEntryByPath(std::list<std::string> const& path) {
		std::shared_ptr<Model_Entry> result = nullptr;
		if (path.size() == 0) { // top level oep
			result = this->root;
		} else {
			for (auto pathPart : path) {
				result = this->getEntryByName(pathPart, result != nullptr ? result->subEntries : this->entries());
				if (result == nullptr)
					return nullptr;
			}
		}
		return result;
	}

	public: std::shared_ptr<Model_Entry> getEntryByName(
		std::string const& name,
		std::list<std::shared_ptr<Model_Entry>>& parentList
	) {
		std::list<std::shared_ptr<Model_Entry>> results;
		for (auto entry : parentList) {
			if (entry->name == name)
				results.push_back(entry);
		}
		if (results.size() == 1) {
			return results.front();
		}
		return nullptr;
	}

	public: std::shared_ptr<Model_Entry> getEntryByHash(
		std::string const& hash,
		std::list<std::shared_ptr<Model_Entry>>& parentList
	) {
		for (auto entry : parentList) {
			if (entry->type == Model_Entry::MENUENTRY && entry->content != "" && Helper::md5(entry->content) == hash) {
				return entry;
			} else if (entry->type == Model_Entry::SUBMENU) {
				auto result = this->getEntryByHash(hash, entry->subEntries);
				if (result != nullptr) {
					return result;
				}
			}
		}
		return nullptr;
	}

	public: std::shared_ptr<Model_Entry> getPlaintextEntry()
	{
		for (auto entry : this->entries()) {
			if (entry->type == Model_Entry::PLAINTEXT) {
				return entry;
			}
		}
		return nullptr;
	}

	public: void moveToBasedir(std::string const& cfg_dir)
	{
		std::string newPath;
		if (isInScriptDir(cfg_dir)){
			newPath = cfg_dir+"/PS_"+this->fileName.substr((cfg_dir+"/proxifiedScripts/").length());
		}
		else {
			newPath = cfg_dir+"/DS_"+this->fileName.substr(cfg_dir.length()+1);
		}
		Helper::assert_filepath_empty(newPath, __FILE__, __LINE__);
		int renameSuccess = rename(this->fileName.c_str(), newPath.c_str());
		if (renameSuccess == 0)
			this->fileName = newPath;
	}

	//moves the file from any location to grub.d and adds the prefix PS_ (proxified Script) or DS_ (default script)
	public: bool moveFile(std::string const& newPath, short int permissions = -1)
	{
		Helper::assert_filepath_empty(newPath, __FILE__, __LINE__);
		int rename_success = rename(this->fileName.c_str(), newPath.c_str());
		if (rename_success == 0){
			this->fileName = newPath;
			if (permissions != -1)
				chmod(this->fileName.c_str(), permissions);
			return true;
		}
		return false;
	}

	public: std::list<std::string> buildPath(std::shared_ptr<Model_Entry> entry, std::shared_ptr<Model_Entry> parent) const
	{
		if (entry == this->root) { // return an empty list if it's the root entry!
			return std::list<std::string>();
		}
		auto& list = parent ? parent->subEntries : this->entries();
		for (auto loop_entry : list) {
			if (loop_entry == entry) {
				std::list<std::string> result;
				result.push_back(loop_entry->name);
				return result;
			}
			if (loop_entry->type == Model_Entry::SUBMENU) {
				try {
					std::list<std::string> result = this->buildPath(entry, loop_entry);
					result.push_front(loop_entry->name);
					return result;
				} catch (ItemNotFoundException const& e) {
					//continue
				}
			}
		}
		throw ItemNotFoundException("entry not found inside of specified parent", __FILE__, __LINE__);
	}

	public: std::list<std::string> buildPath(std::shared_ptr<Model_Entry> entry) const
	{
		return this->buildPath(entry, nullptr);
	}

	public: std::string buildPathString(std::shared_ptr<Model_Entry> entry, bool withOtherEntriesPlaceholder = false) const
	{
		std::string result;
		std::list<std::string> list = this->buildPath(entry, NULL);
		for (std::list<std::string>::iterator iter = list.begin(); iter != list.end(); iter++) {
			if (result != "") {
				result += "/";
			}
			result += "'"+Helper::str_replace("'", "''", *iter)+"'";
		}
	
		if (withOtherEntriesPlaceholder) {
			result += result.size() ? "/*" : "*";
		}
		return result;
	}

	public: bool hasEntry(
		std::shared_ptr<Model_Entry const> entry,
		std::shared_ptr<Model_Entry const> parent = nullptr
	) const {
		if (parent == nullptr && this->root == entry) { // check toplevel entry
			return true;
		}
	
		auto const& list = parent ? parent->subEntries : this->entries();
	
		for (auto loop_entry : list) {
			if (loop_entry == entry) {
				return true;
			}
			if (loop_entry->type == Model_Entry::SUBMENU) {
				bool has = this->hasEntry(entry, loop_entry);
				if (has) {
					return true;
				}
			}
		}
		return false;
	}

	public: void deleteEntry(std::shared_ptr<Model_Entry> entry, std::shared_ptr<Model_Entry> parent = nullptr)
	{
		if (parent == nullptr) {
			parent = this->root;
		}
		for (auto iter = parent->subEntries.begin(); iter != parent->subEntries.end(); iter++) {
			if (*iter == entry) {
				parent->subEntries.erase(iter);
				this->root->isModified = true;
				return;
			} else if (iter->get()->subEntries.size()) {
				try {
					this->deleteEntry(entry, *iter);
					return; // if no exception the entry has been deleted
				} catch (ItemNotFoundException const& e) {}
			}
		}
		throw ItemNotFoundException("entry for deletion not found");
	}

	public: bool deleteFile()
	{
		int success = unlink(this->fileName.c_str());
		if (success == 0){
			this->fileName = "";
			return true;
		}
		else
			return false;
	}

	public: operator ArrayStructure() const
	{
		ArrayStructure result;

		result["name"] = this->name;
		result["fileName"] = this->fileName;
		result["isCustomScript"] = this->isCustomScript;
		result["root"] = ArrayStructure(*this->root);

		return result;
	}

	public: static int extractIndexFromPath(std::string const& path, std::string const& cfgDirPath) {
		if (path.substr(0, cfgDirPath.length()) == cfgDirPath) {
			std::string subPath = path.substr(cfgDirPath.length() + 1); // remove path
			std::string prefix = subPath.substr(0, 2);
			if (prefix.length() == 2 && prefix[0] >= '0' && prefix[0] <= '9' && prefix[1] >= '0' && prefix[1] <= '9') {
				int prefixNum = (prefix[0] - '0') * 10 + (prefix[1] - '0');
				return prefixNum;
			}
		}
		throw InvalidStringFormatException("unable to parse index from " + path, __FILE__, __LINE__);
	}

};

#endif
