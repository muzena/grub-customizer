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

#ifndef GRUB_CUSTOMIZER_PROXY_INCLUDED
#define GRUB_CUSTOMIZER_PROXY_INCLUDED
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <memory>
#include "../lib/Exception.hpp"
#include "../lib/ArrayStructure.hpp"
#include "../lib/Type.hpp"
#include "EntryPathBuilderImpl.hpp"
#include "ProxyScriptData.hpp"
#include "Rule.hpp"
#include "Script.hpp"

class Model_Proxy : public Proxy
{
	public: std::list<std::shared_ptr<Model_Rule>> rules;
	public: int index;
	public: short int permissions;
	public: std::string fileName; //may be the same as Script::fileName
	public: std::shared_ptr<Model_Script> dataSource;

	private: std::map<std::shared_ptr<Model_Script>, std::list<std::list<std::string> > > __idPathList; //to be used by sync()
	private: std::map<std::shared_ptr<Model_Script>, std::list<std::list<std::string> > > __idPathList_OtherEntriesPlaceHolders; //to be used by sync()

	public: Model_Proxy()
		: dataSource(nullptr), permissions(0755), index(90)
	{
	}

	public: Model_Proxy(std::shared_ptr<Model_Script> dataSource, bool activateRules = true)
		: dataSource(dataSource), permissions(0755), index(90)
	{
		rules.push_back(
			std::make_shared<Model_Rule>(
				Model_Rule::OTHER_ENTRIES_PLACEHOLDER,
				std::list<std::string>(),
				"*",
				activateRules
			)
		);
		sync(true, true);
	}

	public: bool isExecutable() const
	{
		return this->permissions & 0111;
	}

	public: void set_isExecutable(bool value)
	{
		if (value)
			permissions |= 0111;
		else
			permissions &= ~0111;
	}

	public: static std::list<std::shared_ptr<Model_Rule>> parseRuleString(
		const char** ruleString,
		std::string const& cfgDirPrefix
	) {
		std::list<std::shared_ptr<Model_Rule>> rules;
	
		bool inString = false, inAlias = false, inHash = false, inFromClause = false;
		std::string name;
		std::string hash;
		std::list<std::string> path;
		bool visible = false;
		const char* iter = nullptr;
		for (iter = *ruleString; *iter && (*iter != '}' || inString || inAlias || inFromClause); iter++) {
			if (!inString && *iter == '+') {
				visible = true;
			} else if (!inString && *iter == '-') {
				visible = false;
			} else if (*iter == '\'' && iter[1] != '\'') {
				inString = !inString;
				if (iter[1] != '/') {
					if (!inString){
						if (inAlias) {
							rules.back()->outputName = name;
						} else if (inFromClause) {
							rules.back()->__sourceScriptPath = cfgDirPrefix + name;
						} else {
							path.push_back(name);
							rules.push_back(std::make_shared<Model_Rule>(Model_Rule::NORMAL, path, visible));
							path.clear();
						}
						inAlias = false;
						inFromClause = false;
					}
					name = "";
				}
			} else if (!inString && *iter == '*') {
				rules.push_back(std::make_shared<Model_Rule>(Model_Rule::OTHER_ENTRIES_PLACEHOLDER, path, "*", visible));
				path.clear();
			} else if (!inString && *iter == '#' && *++iter == 't' && *++iter == 'e' && *++iter == 'x' && *++iter == 't') {
				path.push_back("#text");
				rules.push_back(std::make_shared<Model_Rule>(Model_Rule::PLAINTEXT, path, "#text", visible));
				path.clear();
				name = "";
			} else if (inString) {
				name += *iter;
				if (*iter == '\'')
					iter++;
			} else if (inHash && *iter != '~') {
				hash += *iter;
			} else if (!inString && !inAlias && !inFromClause && *iter == 'a' && *++iter == 's') {
				inAlias = true;
			} else if (!inString && !inAlias && !inFromClause && *iter == 'f' && *++iter == 'r' && *++iter == 'o' && *++iter == 'm') {
				inFromClause = true;
			} else if (!inString && !inAlias && !inFromClause && *iter == '/') {
				path.push_back(name);
				name = "";
			} else if (!inString && !inAlias && !inFromClause && *iter == '{') {
				iter++;
				rules.back()->subRules = Model_Proxy::parseRuleString(&iter, cfgDirPrefix);
				rules.back()->type = Model_Rule::SUBMENU;
			} else if (!inString && *iter == '~') {
				inHash = !inHash;
				if (!inHash) {
					rules.back()->__idHash = hash;
					hash = "";
				}
			}
		}
		*ruleString = iter;
		return rules;
	}

	public: void importRuleString(const char* ruleString, std::string const& cfgDirPrefix)
	{
		rules = Model_Proxy::parseRuleString(&ruleString, cfgDirPrefix);
	}

	public: std::shared_ptr<Model_Rule> getRuleByEntry(
		std::shared_ptr<Model_Entry> const& entry,
		std::list<std::shared_ptr<Model_Rule>>& list,
		Model_Rule::RuleType ruletype
	) {
		for (auto rule : list){
			if (entry == rule->dataSource && rule->type == ruletype)
				return rule;
			else {
				auto result = this->getRuleByEntry(entry, rule->subRules, ruletype);
				if (result) {
					return result;
				}
			}
		}
		return nullptr;
	}

	public: std::list<std::shared_ptr<Model_Rule>> getForeignRules(std::shared_ptr<Model_Rule> parent = nullptr)
	{
		assert(this->dataSource != nullptr);
	
		std::list<std::shared_ptr<Model_Rule>> result;
		auto& list = parent ? parent->subRules : this->rules;
	
		for (auto rule : list) {
			if (rule->dataSource && !this->dataSource->hasEntry(rule->dataSource)) {
				result.push_back(rule);
			}
			if (rule->subRules.size()) {
				auto subResult = this->getForeignRules(rule);
				result.splice(result.end(), subResult);
			}
		}
		return result;
	}

	public: void unsync(std::shared_ptr<Model_Rule> parent = nullptr)
	{
		auto& list = parent ? parent->subRules : this->rules;
		for (auto rule : list) {
			rule->dataSource = nullptr;
			if (rule->subRules.size()) {
				this->unsync(rule);
			}
		}
	}

	public: bool sync(
		bool deleteInvalidRules = true,
		bool expand = true,
		std::map<std::string, std::shared_ptr<Model_Script>> scriptMap = std::map<std::string, std::shared_ptr<Model_Script>>()
	) {
		if (this->dataSource){
			this->sync_connectExisting(nullptr, scriptMap);
			this->sync_connectExistingByHash(nullptr, scriptMap);
			if (expand) {
				this->sync_add_placeholders(nullptr, scriptMap);
				this->sync_expand(scriptMap);
			}
	
			if (deleteInvalidRules)
				this->sync_cleanup(nullptr, scriptMap);
	
			return true;
		}
		else
			return false;
	}

	public: void sync_connectExisting(
		std::shared_ptr<Model_Rule> parent = nullptr,
		std::map<std::string, std::shared_ptr<Model_Script>> scriptMap = std::map<std::string, std::shared_ptr<Model_Script>>()
	) {
		assert(this->dataSource != nullptr);
		if (parent == nullptr) {
			this->__idPathList.clear();
			this->__idPathList_OtherEntriesPlaceHolders.clear();
		}
		auto& list = parent ? parent->subRules : this->rules;
		for (auto rule : list) {
			if (rule->type != Model_Rule::SUBMENU) { // don't sync submenu entries
				std::list<std::string> path = rule->__idpath;
	
				std::shared_ptr<Model_Script> script = nullptr;
				if (rule->__sourceScriptPath == "") { // main dataSource
					script = this->dataSource;
				} else if (scriptMap.size()) {
					assert(scriptMap.find(rule->__sourceScriptPath) != scriptMap.end()); // expecting that the script exists on the map
					script = scriptMap[rule->__sourceScriptPath];
				} else {
					continue; // don't sync foreign entries if scriptMap is empty
				}
	
				if (rule->type != Model_Rule::OTHER_ENTRIES_PLACEHOLDER) {
					this->__idPathList[script].push_back(path);
				} else {
					this->__idPathList_OtherEntriesPlaceHolders[script].push_back(path);
				}
	
				rule->dataSource = script->getEntryByPath(path);
	
			} else if (rule->subRules.size()) {
				this->sync_connectExisting(rule, scriptMap);
			}
		}
	}

	public: void sync_connectExistingByHash(
		std::shared_ptr<Model_Rule> parent = nullptr,
		std::map<std::string, std::shared_ptr<Model_Script>> scriptMap = std::map<std::string, std::shared_ptr<Model_Script>>()
	) {
		assert(this->dataSource != nullptr);
		auto& list = parent ? parent->subRules : this->rules;
		for (auto rule : list) {
			if (rule->dataSource == nullptr && rule->__idHash != "") {
				std::shared_ptr<Model_Script> script = nullptr;
				if (rule->__sourceScriptPath == "") {
					script = this->dataSource;
				} else if (scriptMap.size()) {
					assert(scriptMap.find(rule->__sourceScriptPath) != scriptMap.end()); // expecting that the script exists on the map
					script = scriptMap[rule->__sourceScriptPath];
				} else {
					continue; // don't sync foreign entries if scriptMap is empty
				}
				rule->dataSource = script->getEntryByHash(rule->__idHash, script->entries());
				if (rule->dataSource) {
					this->__idPathList[script].push_back(script->buildPath(rule->dataSource));
				}
			}
			if (rule->subRules.size()) {
				this->sync_connectExistingByHash(rule, scriptMap);
			}
		}
	}

	public: void sync_add_placeholders(
		std::shared_ptr<Model_Rule> parent = nullptr,
		std::map<std::string, std::shared_ptr<Model_Script>> scriptMap = std::map<std::string, std::shared_ptr<Model_Script>>()
	) {
		assert(parent == nullptr || parent->dataSource != nullptr);
	
		std::list<std::string> path = parent ? this->dataSource->buildPath(parent->dataSource) : std::list<std::string>();
		//find out if currentPath is on the blacklist
		bool eop_is_blacklisted = false;
	
		for (auto oepPath : this->__idPathList_OtherEntriesPlaceHolders[this->dataSource]) {
			if (oepPath == path) {
				eop_is_blacklisted = true;
				break;
			}
		}
	
		auto& list = parent ? parent->subRules : this->rules;
		if (!eop_is_blacklisted) {
			auto newRule = std::make_shared<Model_Rule>(
				Model_Rule::OTHER_ENTRIES_PLACEHOLDER,
				parent ? this->dataSource->buildPath(parent->dataSource) : std::list<std::string>(),
				"*",
				true
			);
			newRule->dataSource = this->dataSource->getEntryByPath(path);
			list.push_front(newRule);
			this->__idPathList_OtherEntriesPlaceHolders[this->dataSource].push_back(path);
		}
	
		//sub entries (recursion)
		for (auto rule : list) {
			if (rule->dataSource && rule->type == Model_Rule::SUBMENU) {
				this->sync_add_placeholders(rule);
			}
		}
	}

	public: void sync_expand(
		std::map<std::string,
		std::shared_ptr<Model_Script>> scriptMap = std::map<std::string, std::shared_ptr<Model_Script>>()
	) {
		assert(this->dataSource != nullptr);
		for (auto scriptMapEnt : this->__idPathList_OtherEntriesPlaceHolders) {
			for (auto oepPath : this->__idPathList_OtherEntriesPlaceHolders[scriptMapEnt.first]) {
				auto dataSource = scriptMapEnt.first->getEntryByPath(oepPath);
				if (dataSource) {
					auto oep = this->getRuleByEntry(dataSource, this->rules, Model_Rule::OTHER_ENTRIES_PLACEHOLDER);
					assert(oep != nullptr);
					auto parentRule = this->getParentRule(oep);
					auto& dataTarget = parentRule ? parentRule->subRules : this->rules;
	
					auto dataTargetIter = dataTarget.begin();
					while (dataTargetIter != dataTarget.end()
						&& !(dataTargetIter->get()->type == Model_Rule::OTHER_ENTRIES_PLACEHOLDER
							&& dataTargetIter->get()->__idpath == oepPath
							&& ((dataTargetIter->get()->__sourceScriptPath != ""
									&& scriptMap.size()
									&& scriptMap[dataTargetIter->get()->__sourceScriptPath] == scriptMapEnt.first)
								|| (dataTargetIter->get()->__sourceScriptPath == ""
									&& scriptMapEnt.first == this->dataSource)
								)
							)
						) {
						dataTargetIter++;
					}
					std::list<std::shared_ptr<Model_Rule>> newRules;
					for (auto subEntry : dataSource->subEntries){
						auto relatedRule = this->getRuleByEntry(subEntry, this->rules, Model_Rule::NORMAL);
						auto relatedRulePt = this->getRuleByEntry(subEntry, this->rules, Model_Rule::PLAINTEXT);
						auto relatedRuleOep = this->getRuleByEntry(subEntry, this->rules, Model_Rule::OTHER_ENTRIES_PLACEHOLDER);
						if (!relatedRule && !relatedRuleOep && !relatedRulePt){
							newRules.push_back(
								std::make_shared<Model_Rule>(
									subEntry,
									dataTargetIter->get()->isVisible,
									scriptMapEnt.first,
									this->__idPathList[scriptMapEnt.first],
									scriptMapEnt.first->buildPath(subEntry)
								)
							); //generate rule for given entry
						}
					}
					dataTargetIter++;
					dataTarget.splice(dataTargetIter, newRules);
				}
			}
		}
	}

	public: void sync_cleanup(
		std::shared_ptr<Model_Rule> parent = nullptr,
		std::map<std::string, std::shared_ptr<Model_Script>> scriptMap = std::map<std::string, std::shared_ptr<Model_Script>>()
	) {
		auto& list = parent ? parent->subRules : this->rules;
	
		bool done = false;
		do {
			bool listModified = false;
			for (auto iter = list.begin(); !listModified && iter != list.end(); iter++) {
				if (!((iter->get()->type == Model_Rule::NORMAL && iter->get()->dataSource) ||
					  (iter->get()->type == Model_Rule::SUBMENU && iter->get()->subRules.size()) ||
					  (iter->get()->type == Model_Rule::OTHER_ENTRIES_PLACEHOLDER && iter->get()->dataSource) ||
					  (iter->get()->type == Model_Rule::PLAINTEXT && iter->get()->dataSource))) {
					if (iter->get()->__sourceScriptPath == "" || scriptMap.size()) {
						list.erase(iter);
						listModified = true; //after ereasing something we have to create a new iterator
					}
				} else { //check contents
					this->sync_cleanup(*iter, scriptMap);
				}
			}
	
			if (!listModified)
				done = true;
		} while (!done);
	}

	public: bool isModified(
		std::shared_ptr<Model_Rule> parentRule = nullptr,
		std::shared_ptr<Model_Entry> parentEntry = nullptr
	) const {
		assert(this->dataSource != nullptr);
		bool result = false;
	
		auto& rlist = parentRule ? parentRule->subRules : this->rules;
		auto& elist = parentEntry ? parentEntry->subEntries : this->dataSource->entries();
		if (rlist.size()-1 == elist.size()){ //rules contains the other entries placeholder, so there is one more entry
			auto ruleIter = rlist.begin();
			auto entryIter = elist.begin();
			if (ruleIter->get()->type == Model_Rule::OTHER_ENTRIES_PLACEHOLDER){ //the first element is the OTHER_ENTRIES_PLACEHOLDER by default.
				result = !ruleIter->get()->isVisible; //If not visible, it's modified…
				ruleIter++;
			} else {
				result = true;
			}
			while (!result && ruleIter != rlist.end() && entryIter != elist.end()){
				// type compare
				if ((ruleIter->get()->type == Model_Rule::NORMAL && entryIter->get()->type != Model_Entry::MENUENTRY)
					|| (ruleIter->get()->type == Model_Rule::PLAINTEXT && entryIter->get()->type != Model_Entry::PLAINTEXT)
					|| (ruleIter->get()->type == Model_Rule::SUBMENU && entryIter->get()->type != Model_Entry::SUBMENU)) {
					result = true;
				} else if (ruleIter->get()->outputName != entryIter->get()->name || !ruleIter->get()->isVisible) { // data compare
					result = true;
				} else if (ruleIter->get()->type == Model_Rule::SUBMENU) { // submenu check
					result = this->isModified(*ruleIter, *entryIter);
				}
	
				ruleIter++;
				entryIter++;
			}
		} else {
			result = true;
		}
		return result;
	}

	public: bool deleteFile()
	{
		assert(Model_ProxyScriptData::is_proxyscript(this->fileName));
		int success = unlink(this->fileName.c_str());
		if (success == 0){
			this->fileName = "";
			return true;
		} else
			return false;
	}

	public: std::list<std::string> getScriptList(
		std::map<std::shared_ptr<Model_Entry>, std::shared_ptr<Model_Script>> const& entrySourceMap,
		std::map<std::shared_ptr<Model_Script>, std::string> const& scriptTargetMap
	) const {
		std::map<std::string, Nothing> uniqueList; // the pointer (value) is just a dummy
		for (auto entrySource : entrySourceMap) {
			uniqueList[scriptTargetMap.find(entrySource.second)->second] = Nothing();
		}
		std::list<std::string> result;
		result.push_back(scriptTargetMap.find(this->dataSource)->second); // the own script must be the first entry
		for (auto uniqueListItem : uniqueList) {
			result.push_back(uniqueListItem.first);
		}
		return result;
	}

	public: bool generateFile(
		std::string const& path,
		int cfg_dir_prefix_length,
		std::string const& cfg_dir_noprefix,
		std::map<std::shared_ptr<Model_Entry>, std::shared_ptr<Model_Script>> entrySourceMap,
		std::map<std::shared_ptr<Model_Script>, std::string> const& scriptTargetMap
	) {
		if (this->dataSource){
			FILE* proxyFile = fopen(path.c_str(), "w");
			if (proxyFile){
				this->fileName = path;
				fputs("#!/bin/sh\n#THIS IS A GRUB PROXY SCRIPT\n", proxyFile);
				std::list<std::string> scripts = this->getScriptList(entrySourceMap, scriptTargetMap);
				if (scripts.size() == 1) { // single script
					fputs(("'"+this->dataSource->fileName.substr(cfg_dir_prefix_length)+"'").c_str(), proxyFile);
				} else { // multi script
					fputs("sh -c '", proxyFile);
					for (std::list<std::string>::iterator scriptIter = scripts.begin(); scriptIter != scripts.end(); scriptIter++) {
						fputs(("echo \"### BEGIN "+(*scriptIter).substr(cfg_dir_prefix_length)+" ###\";\n").c_str(), proxyFile);
						fputs(("\""+(*scriptIter).substr(cfg_dir_prefix_length)+"\";\n").c_str(), proxyFile);
						fputs(("echo \"### END "+(*scriptIter).substr(cfg_dir_prefix_length)+" ###\";").c_str(), proxyFile);
						if (&*scriptIter != &scripts.back()) {
							fputs("\n", proxyFile);
						}
					}
					fputs("'", proxyFile);
				}
				fputs((" | "+cfg_dir_noprefix+"/bin/grubcfg_proxy \"").c_str(), proxyFile);
				for (auto rule : this->rules) {
					assert(this->dataSource != nullptr);
					Model_EntryPathBuilderImpl entryPathBuilder(this->dataSource);
					entryPathBuilder.setScriptTargetMap(scriptTargetMap);
					entryPathBuilder.setEntrySourceMap(entrySourceMap);
					entryPathBuilder.setPrefixLength(cfg_dir_prefix_length);
					fputs((rule->toString(entryPathBuilder)+"\n").c_str(), proxyFile); //write rule
				}
				fputs("\"", proxyFile);
				if (scripts.size() > 1) {
					fputs(" multi", proxyFile);
				}
				fclose(proxyFile);
				chmod(path.c_str(), this->permissions);
				return true;
			}
		}
		return false;
	}

	//before running this function, the related script file must be saved!
	public: std::string getScriptName() {
		if (this->dataSource) {
			return this->dataSource->name;
		} else {
			return "?";
		}
	}

	public: std::list<std::shared_ptr<Model_Rule>>::iterator getNextVisibleRule(
		std::list<std::shared_ptr<Model_Rule>>::iterator base,
		int direction
	) {
		assert(direction == -1 || direction == 1);
		std::shared_ptr<Model_Rule> parent = nullptr;
		try {
			parent = this->getParentRule(*base);
		} catch (ItemNotFoundException const& e) {} // leave parent in nullptr state
		auto& list = this->getRuleList(parent);
	
		do {
			if (direction == 1) {
				base++;
			} else {
				base--;
			}
		} while (base != list.end() && !base->get()->isVisible);
		if (base == list.end()) {
			throw NoMoveTargetException("no move target found inside of this proxy", __FILE__, __LINE__);
		}
		return base;
	}

	public: std::shared_ptr<Model_Rule> splitSubmenu(std::shared_ptr<Model_Rule> position) {
		std::shared_ptr<Model_Rule> parent = this->getParentRule(position);
	
		// search items before and after the submenu
		std::list<std::shared_ptr<Model_Rule>> rulesBefore;
		std::list<std::shared_ptr<Model_Rule>> rulesAfter;
	
		bool isBehindChildtem = false;
		for (auto rule : parent->subRules) {
			if (rule != position) {
				if (!isBehindChildtem) {
					rulesBefore.push_back(rule);
				} else {
					rulesAfter.push_back(rule);
				}
			} else {
				rulesAfter.push_back(rule);
				isBehindChildtem = true;
			}
		}
		auto oldSubmenu = parent;
		oldSubmenu->subRules.clear();
	
		std::list<std::shared_ptr<Model_Rule>>* list = nullptr;
		auto parentRule = this->getParentRule(parent);
		if (parentRule) {
			list = &parentRule->subRules;
		} else {
			list = &this->rules;
		}
		assert(list != nullptr);
		// add the rules before and/or after to new submenus
		if (rulesBefore.size()) {
			auto newSubmenu = std::make_shared<Model_Rule>(*oldSubmenu);
			newSubmenu->subRules = rulesBefore;
			list->insert(this->getListIterator(parent, *list), newSubmenu);
		}
	
	
		auto newSubmenu = std::make_shared<Model_Rule>(*oldSubmenu);
		newSubmenu->subRules = rulesAfter;
		auto iter = this->getListIterator(parent, *list);
		iter++;
		auto insertPos = list->insert(iter, newSubmenu);
	
		// remove the submenu
		list->erase(this->getListIterator(parent, *list));
	
		return insertPos->get()->subRules.front();
	}

	public: std::shared_ptr<Model_Rule> createSubmenu(std::shared_ptr<Model_Rule> position) {
		auto& list = this->getRuleList(this->getParentRule(position));

		auto posIter = this->getListIterator(position, list);
		auto insertPos = list.insert(
			posIter,
			std::make_shared<Model_Rule>(Model_Rule::SUBMENU, std::list<std::string>(), "", true)
		);
	
		return *insertPos;
	}

	public: bool ruleIsFromOwnScript(std::shared_ptr<Model_Rule> rule) const {
		assert(this->dataSource != nullptr);
		assert(rule->dataSource != nullptr);
		if (this->dataSource->hasEntry(rule->dataSource)) {
			return true;
		} else {
			return false;
		}
	}

	public: void removeForeignChildRules(std::shared_ptr<Model_Rule> parent) {
		bool loopRestartRequired = false;
		do { // required to restart the loop after an entry has been removed
			loopRestartRequired = false;
			for (auto iter = parent->subRules.begin(); iter != parent->subRules.end(); iter++) {
				if (iter->get()->dataSource) {
					if (!this->ruleIsFromOwnScript(*iter)) {
						parent->subRules.erase(iter);
						loopRestartRequired = true;
						break;
					}
				} else if (iter->get()->subRules.size()) {
					this->removeForeignChildRules(*iter);
					if (iter->get()->subRules.size() == 0) { // if this submenu is empty now, remove it
						parent->subRules.erase(iter);
						loopRestartRequired = true;
						break;
					}
				}
			}
		} while (loopRestartRequired);
	}

	public: void removeEquivalentRules(std::shared_ptr<Model_Rule> base) {
		if (base->dataSource) {
			auto eqRule = this->getRuleByEntry(base->dataSource, this->rules, base->type);
			if (eqRule) {
				this->removeRule(eqRule);
			}
		} else if (base->subRules.size()) {
			for (auto subRule : base->subRules) {
				this->removeEquivalentRules(subRule);
			}
		}
	}

	public: void removeRule(std::shared_ptr<Model_Rule> rule) {
		assert(rule != nullptr);
		std::shared_ptr<Model_Rule> parent = nullptr;
		int rlist_size = 0;
		do {
			try {
				parent = this->getParentRule(rule);
			} catch (ItemNotFoundException const& e) {
				parent = nullptr;
			}
			auto& rlist = this->getRuleList(parent);
			auto iter = this->getListIterator(rule, rlist);
			rlist.erase(iter);
	
			rule = parent; // go one step up to remove this rule if empty
			rlist_size = rlist.size();
		} while (rlist_size == 0 && parent != nullptr); // delete all the empty submenus above
	}

	public: std::list<std::shared_ptr<Model_Rule>>::iterator getListIterator(
		std::shared_ptr<Model_Rule> needle,
		std::list<std::shared_ptr<Model_Rule>>& haystack
	) {
		for (auto iter = haystack.begin(); iter != haystack.end(); iter++) {
			if (*iter == needle)
				return iter;
		}
	
		throw ItemNotFoundException("specified rule not found", __FILE__, __LINE__);
	}

	public: std::shared_ptr<Model_Rule> getParentRule(
		std::shared_ptr<Model_Rule> child,
		std::shared_ptr<Model_Rule> root = nullptr
	) {
		auto& list = root ? root->subRules : this->rules;
		for (auto rule : list) {
			if (rule == child)
				return root;
			else if (rule->subRules.size()) {
				std::shared_ptr<Model_Rule> parentRule = nullptr;
				try {
					parentRule = this->getParentRule(child, rule);
				} catch (ItemNotFoundException const& e) {
					// do nothing
				}
				if (parentRule) {
					return parentRule;
				}
			}
		}
		throw ItemNotFoundException("specified rule not found", __FILE__, __LINE__);
	}

	public: std::list<std::shared_ptr<Model_Rule>>& getRuleList(std::shared_ptr<Model_Rule> parentElement)
	{
		if (parentElement)
			return parentElement->subRules;
		else
			return this->rules;
	}

	public: bool hasVisibleRules(std::shared_ptr<Model_Rule> parent = nullptr) const {
		auto& list = parent ? parent->subRules : this->rules;
		for (auto iter = list.begin(); iter != list.end(); iter++) {
			if (iter->get()->isVisible) {
				if (iter->get()->type == Model_Rule::SUBMENU) {
					bool has = this->hasVisibleRules(*iter);
					if (has) {
						return true;
					}
				} else {
					return true;
				}
			}
		}
		return false;
	}

	public: std::shared_ptr<Model_Rule> getVisibleRuleForEntry(
		std::shared_ptr<Model_Entry> const& entry,
		std::shared_ptr<Model_Rule> parent = nullptr
	) {
		auto& list = parent ? parent->subRules : this->rules;
		for (auto rule : list) {
			if (rule->isVisible && rule->dataSource == entry) {
				return rule;
			}
			auto subResult = this->getVisibleRuleForEntry(entry, rule);
			if (subResult) {
				return subResult;
			}
		}
		return nullptr;
	}

	public: std::list<std::shared_ptr<Model_Rule>> getVisibleRulesByType(
		Model_Rule::RuleType type,
		std::shared_ptr<Model_Rule> parent = nullptr
	) {
		std::list<std::shared_ptr<Model_Rule>> result;
		auto& list = parent ? parent->subRules : this->rules;
	
		for (auto rule : list) {
			if (rule->isVisible) {
				if (rule->type == type) {
					result.push_back(rule);
				}
				if (rule->subRules.size()) {
					auto subResult = this->getVisibleRulesByType(type, rule);
					result.splice(result.end(), subResult);
				}
			}
		}
	
		return result;
	}

	public: operator ArrayStructure() const {
		ArrayStructure result;
		result["rules"].isArray = true;
		int ruleIterPos = 0;
		for (auto rule : this->rules) {
			result["rules"][ruleIterPos] = *rule;
			ruleIterPos++;
		}
		result["index"] = this->index;
		result["permissions"] = this->permissions;
		result["fileName"] = this->fileName;
		result["dataSource"] = this->dataSource.get();
		result["__idPathList"].isArray = true;
		{
			for (auto idPath : this->__idPathList) {
				result["__idPathList"]["k"] = *idPath.first;
				int i = 0;
				for (auto idPathPart : idPath.second) {
					result["__idPathList"]["v"][i] = ArrayStructure(idPathPart);
					i++;
				}
			}
		}
		result["__idPathList_OtherEntriesPlaceHolders"].isArray = true;
		{
			for (auto oepPath : this->__idPathList_OtherEntriesPlaceHolders) {
				result["__idPathList_OtherEntriesPlaceHolders"]["k"] = *oepPath.first;
				int i = 0;
				for (auto oepPathPart : oepPath.second) {
					result["__idPathList_OtherEntriesPlaceHolders"]["v"][i] = ArrayStructure(oepPathPart);
					i++;
				}
			}
		}

		return result;
	}

	private: static void adjustIterator(std::list<Model_Rule>::iterator& iter, int adjustment) {
		if (adjustment > 0) {
			for (int i = 0; i < adjustment; i++) {
				iter++;
			}
		} else {
			for (int i = 0; i > adjustment; i--) {
				iter--;
			}
		}
	}

};

#endif
