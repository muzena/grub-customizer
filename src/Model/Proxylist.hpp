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

#ifndef GRUB_CUSTOMIZER_PROXYLIST_INCLUDED
#define GRUB_CUSTOMIZER_PROXYLIST_INCLUDED
#include <list>
#include <sstream>
#include <memory>
#include "../lib/Trait/LoggerAware.hpp"
#include "../lib/Exception.hpp"
#include "../lib/ArrayStructure.hpp"
#include "Proxy.hpp"

struct Model_Proxylist_Item {
	std::string labelPathValue;
	std::string labelPathLabel;
	std::string numericPathValue;
	std::string numericPathLabel;
};
class Model_Proxylist : public std::list<std::shared_ptr<Model_Proxy>>, public Trait_LoggerAware
{
	public: std::list<std::shared_ptr<Model_Proxy>> trash; //removed proxies

	public: std::list<std::shared_ptr<Model_Proxy>> getProxiesByScript(std::shared_ptr<Model_Script> script)
	{
		std::list<std::shared_ptr<Model_Proxy>> result;
		for (auto proxy : *this) {
			if (proxy->dataSource == script)
				result.push_back(proxy);
		}
		return result;
	}

	public: std::list<std::shared_ptr<Model_Proxy>> getProxiesByScript(std::shared_ptr<Model_Script> script) const
	{
		std::list<std::shared_ptr<Model_Proxy>> result;
		for (auto proxy : *this){
			if (proxy->dataSource == script)
				result.push_back(proxy);
		}
		return result;
	}

	public: std::list<std::shared_ptr<Model_Rule>> getForeignRules()
	{
		std::list<std::shared_ptr<Model_Rule>> result;
	
		for (auto proxy : *this) {
			auto subResult = proxy->getForeignRules();
			result.splice(result.end(), subResult);
		}
	
		return result;
	}

	//relatedScript = NULL: sync all proxies, otherwise only sync proxies wich target the given Script
	public: void sync_all(
		bool deleteInvalidRules = true,
		bool expand = true,
		std::shared_ptr<Model_Script> relatedScript = nullptr,
		std::map<std::string, std::shared_ptr<Model_Script>> scriptMap = std::map<std::string, std::shared_ptr<Model_Script>>()
	) {
		for (auto proxy : *this) {
			if (relatedScript == nullptr || proxy->dataSource == relatedScript)
				proxy->sync(deleteInvalidRules, expand, scriptMap);
		}	
	}

	public: void unsync_all()
	{
		for (auto proxy : *this) {
			proxy->unsync();
		}
	}

	public: bool proxyRequired(std::shared_ptr<Model_Script> script) const
	{
		auto plist = this->getProxiesByScript(script);
		if (plist.size() == 1){
			return plist.front()->isModified();
		}
		else
			return true;
	}

	public: void deleteAllProxyscriptFiles()
	{
		for (auto proxy : *this) {
			if (proxy->fileName != "" && proxy->dataSource && proxy->dataSource->fileName != proxy->fileName){
				proxy->deleteFile();
			}
		}
	}

	public: static bool compare_proxies(std::shared_ptr<Model_Proxy> const& a, std::shared_ptr<Model_Proxy> const& b)
	{
		if (a->index != b->index) {
			return a->index < b->index;
		} else {
			if (a->dataSource != nullptr && b->dataSource != nullptr) {
				return a->dataSource->name < b->dataSource->name;
			} else {
				return true;
			}
		}
	}

	public: void sort()
	{
		std::list<std::shared_ptr<Model_Proxy>>::sort(Model_Proxylist::compare_proxies);
	}

	public: void deleteProxy(std::shared_ptr<Model_Proxy> proxyPointer) {
		for (auto proxyIter = this->begin(); proxyIter != this->end(); proxyIter++) {
			if (*proxyIter == proxyPointer){
				//if the file must be deleted when saving, move it to trash
				if (proxyPointer->fileName != "" && proxyPointer->dataSource && proxyPointer->fileName != proxyPointer->dataSource->fileName)
					this->trash.push_back(proxyPointer);
				//remove the proxy object
				this->erase(proxyIter);
				break;
			}
		}
	}

	public: void clearTrash()
	{
		for (auto trashedProxy : this->trash){
			if (trashedProxy->fileName != "") {
				trashedProxy->deleteFile();
			}
		}
	}

	public: std::list<Model_Proxylist_Item> generateEntryTitleList() const
	{
		std::list<Model_Proxylist_Item> result;
		int offset = 0;
		for (auto proxy : *this) {
			if (proxy->isExecutable()){
				std::list<Model_Proxylist_Item> subList = Model_Proxylist::generateEntryTitleList(proxy->rules, "", "", "", &offset);
				result.splice(result.end(), subList);
			}
		}
		return result;
	}

	public: std::list<std::string> getToplevelEntryTitles() const
	{
		std::list<std::string> result;
		for (auto proxy : *this) {
			if (proxy->isExecutable()){
				for (auto rule : proxy->rules) {
					if (rule->isVisible && rule->type == Model_Rule::NORMAL) {
						result.push_back(rule->outputName);
					}
				}
			}
		}
		return result;
	}

	public: static std::list<Model_Proxylist_Item> generateEntryTitleList(
		std::list<std::shared_ptr<Model_Rule>> const& parent,
		std::string const& labelPathPrefix,
		std::string const& numericPathPrefix,
		std::string const& numericPathLabelPrefix,
		int* offset = nullptr
	) {
		std::list<Model_Proxylist_Item> result;
		int i = (offset != nullptr ? *offset : 0);
		for (auto rule : parent){
			if (rule->isVisible && (rule->type == Model_Rule::NORMAL || rule->type == Model_Rule::SUBMENU)) {
				std::ostringstream currentNumPath;
				currentNumPath << numericPathPrefix << i;
				std::ostringstream currentLabelNumPath;
				currentLabelNumPath << numericPathLabelPrefix << (i+1);

				bool addedSomething = true;
				if (rule->type == Model_Rule::SUBMENU) {
					std::list<Model_Proxylist_Item> subList = Model_Proxylist::generateEntryTitleList(
						rule->subRules,
						labelPathPrefix + rule->outputName + ">",
						currentNumPath.str() + ">",
						currentLabelNumPath.str() + ">"
					);
					if (subList.size() == 0) {
						addedSomething = false;
					}
					result.splice(result.end(), subList);
				} else {
					Model_Proxylist_Item newItem;
					newItem.labelPathLabel = labelPathPrefix + rule->outputName;
					newItem.labelPathValue = labelPathPrefix + rule->outputName;
					result.push_back(newItem);
				}
				if (addedSomething) {
					i++;
				}
			}
		}
		if (offset != NULL) {
			*offset = i;
		}
		return result;
	}

	public: std::shared_ptr<Model_Proxy> getProxyByRule(
		std::shared_ptr<Model_Rule> rule,
		std::list<std::shared_ptr<Model_Rule>> const& list,
		std::shared_ptr<Model_Proxy> parentProxy
	) {
		for (auto loop_rule : list) {
			if (loop_rule == rule)
				return parentProxy;
			else {
				try {
					return this->getProxyByRule(rule, loop_rule->subRules, parentProxy);
				} catch (ItemNotFoundException const& e) {
					// do nothing
				}
			}
		}
		throw ItemNotFoundException("proxy by rule not found", __FILE__, __LINE__);
	}

	public: std::shared_ptr<Model_Proxy> getProxyByRule(std::shared_ptr<Model_Rule> rule) {
		for (auto proxy : *this) {
			try {
				return this->getProxyByRule(rule, proxy->rules, proxy);
			} catch (ItemNotFoundException const& e) {
				// do nothing
			}
		}
		throw ItemNotFoundException("proxy by rule not found", __FILE__, __LINE__);
	}

	public: std::list<std::shared_ptr<Model_Rule>>::iterator moveRuleToNewProxy(
		std::shared_ptr<Model_Rule> rule,
		int direction,
		std::shared_ptr<Model_Script> dataSource = nullptr
	) {
		auto currentProxy = this->getProxyByRule(rule);
		auto proxyIter = this->begin();

		for (auto proxy : *this) {
			if (proxy == currentProxy) {
				break;
			}
			proxyIter++;
		}
	
		if (direction == 1) {
			proxyIter++;
		}
		if (dataSource == nullptr) {
			dataSource = currentProxy->dataSource;
		}
		auto newProxy = *this->insert(proxyIter, std::make_shared<Model_Proxy>(dataSource, false));
		newProxy->removeEquivalentRules(rule);
		auto movedRule = newProxy->rules.insert(
			direction == -1 ? newProxy->rules.end() : newProxy->rules.begin(),
			std::make_shared<Model_Rule>(*rule)
		);
		rule->setVisibility(false);
	
		if (!currentProxy->hasVisibleRules()) {
			this->deleteProxy(currentProxy);
		}
		return movedRule;
	}

	public: std::list<std::shared_ptr<Model_Rule>>::iterator getNextVisibleRule(std::shared_ptr<Model_Rule> base, int direction) {
		auto proxy = this->getProxyByRule(base);
		auto iter = proxy->getListIterator(base, proxy->getRuleList(proxy->getParentRule(base, nullptr)));
		return this->getNextVisibleRule(iter, direction);
	}

	public: std::list<std::shared_ptr<Model_Rule>>::iterator getNextVisibleRule(
		std::list<std::shared_ptr<Model_Rule>>::iterator base,
		int direction
	) {
		auto proxyIter = this->getIter(this->getProxyByRule(*base));

		bool hasParent = false;
		if (proxyIter->get()->getParentRule(*base)) {
			hasParent = true;
		}

		while (proxyIter != this->end()) {
			try {
				return proxyIter->get()->getNextVisibleRule(base, direction);
			} catch (NoMoveTargetException const& e) {

				if (hasParent) {
					throw NoMoveTargetException("next visible rule not found", __FILE__, __LINE__);
				}

				if (direction == 1) {
					proxyIter++;
					if (proxyIter == this->end()) {
						throw NoMoveTargetException("next visible rule not found", __FILE__, __LINE__);
					}
					base = proxyIter->get()->rules.begin();
				} else {
					proxyIter--;
					if (proxyIter == this->end()) {
						throw NoMoveTargetException("next visible rule not found", __FILE__, __LINE__);
					}
					base = proxyIter->get()->rules.end();
					base--;
				}
				if (base->get()->isVisible) {
					return base;
				}
			}
		}
		throw NoMoveTargetException("next visible rule not found", __FILE__, __LINE__);
	}

	std::list<std::shared_ptr<Model_Proxy>>::iterator getIter(std::shared_ptr<Model_Proxy> proxy) {
		auto iter = this->begin();
		while (iter != this->end()) {
			if (*iter == proxy) {
				break;
			}
			iter++;
		}
		return iter;
	}

	void splitProxy(std::shared_ptr<Model_Proxy> proxyToSplit, std::shared_ptr<Model_Rule> firstRuleOfPart2, int direction) {
		auto iter = this->getIter(proxyToSplit);
		auto sourceProxy = *iter;
		if (direction == 1) {
			iter++;
		}
		auto newProxy = *this->insert(iter, std::make_shared<Model_Proxy>(sourceProxy->dataSource, false));
	
		bool isSecondPart = false;
		if (direction == 1) {
			for (auto ruleIter = sourceProxy->rules.begin(); ruleIter != sourceProxy->rules.end(); ruleIter++) {
				if (*ruleIter == firstRuleOfPart2) {
					isSecondPart = true;
				}
				if (isSecondPart) {
					newProxy->removeEquivalentRules(*ruleIter);
					newProxy->rules.push_back(*ruleIter);
					ruleIter->get()->isVisible = false;
				}
			}
		} else {
			for (auto ruleIter = sourceProxy->rules.rbegin(); ruleIter != sourceProxy->rules.rend(); ruleIter++) {
				if (*ruleIter == firstRuleOfPart2) {
					isSecondPart = true;
				}
				if (isSecondPart) {
					newProxy->removeEquivalentRules(*ruleIter);
					newProxy->rules.push_front(*ruleIter);
					ruleIter->get()->isVisible = false;
				}
			}
		}
	}

	std::shared_ptr<Model_Rule> getVisibleRuleForEntry(std::shared_ptr<Model_Entry> entry) {
		for (auto proxy : *this) {
			if (proxy->isExecutable()) {
				std::shared_ptr<Model_Rule> result = proxy->getVisibleRuleForEntry(entry);
				if (result) {
					return result;
				}
			}
		}
		return NULL;
	}

	bool hasConflicts() const {
		std::map<std::string, bool> resources; // key: combination of number, "_" and name. Value: true if used before
		for (auto proxy : *this) {
			assert(proxy->dataSource); // assume all proxies are having a datasource
			std::ostringstream resourceName;
			resourceName << proxy->index << "_" << proxy->dataSource->name;
			if (resources[resourceName.str()]) {
				return true;
			} else {
				resources[resourceName.str()] = true;
			}
		}
		return false;
	}

	bool hasProxy(std::shared_ptr<Model_Proxy> proxy) {
		for (auto proxy_loop : *this) {
			if (proxy_loop == proxy) {
				return true;
			}
		}
		return false;
	}

	operator ArrayStructure() const {
		ArrayStructure result;
		int trashIterPos = 0;
		result["trash"].isArray = true;
		for (auto trashedProxy : this->trash) {
			result["trash"][trashIterPos] = ArrayStructure(*trashedProxy);
			trashIterPos++;
		}
		int itemsIterPos = 0;
		result["(items)"].isArray = true;
		for (auto proxy : *this) {
			result["(items)"][itemsIterPos] = ArrayStructure(*proxy);
			itemsIterPos++;
		}
		return result;
	}

};

#endif
