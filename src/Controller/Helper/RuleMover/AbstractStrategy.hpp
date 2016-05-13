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

#ifndef INC_Controller_Helper_RuleMover_AbstractStrategy
#define INC_Controller_Helper_RuleMover_AbstractStrategy

#include "../../../Model/Rule.hpp"
#include "../../../Model/ListCfg.hpp"
#include <memory>
#include <string>

class Controller_Helper_RuleMover_AbstractStrategy
{
	public: enum class Direction {
		DOWN = 1,
		UP = -1
	};

	protected: std::string name;

	Controller_Helper_RuleMover_AbstractStrategy(std::string const& name)
		: name(name)
	{}

	public: virtual void move(std::shared_ptr<Model_Rule> rule, Controller_Helper_RuleMover_AbstractStrategy::Direction direction) = 0;

	public: virtual std::string getName()
	{
		return this->name;
	}

	public: virtual ~Controller_Helper_RuleMover_AbstractStrategy(){};


	protected: std::list<std::shared_ptr<Model_Rule>> findVisibleRules(
		std::list<std::shared_ptr<Model_Rule>> ruleList,
		std::shared_ptr<Model_Rule> ruleAlwaysToInclude
	) {
		std::list<std::shared_ptr<Model_Rule>> result;

		for (auto rule : ruleList) {
			if (rule->isVisible || rule == ruleAlwaysToInclude) {
				result.push_back(rule);
			}
		}

		return result;
	}

	protected: std::shared_ptr<Model_Rule> getNextRule(
		std::list<std::shared_ptr<Model_Rule>> list,
		std::shared_ptr<Model_Rule> base,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		auto currentPosition = std::find(list.begin(), list.end(), base);

		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::UP) {
			if (currentPosition == list.begin()) {
				return nullptr;
			}
			currentPosition--;
			return *currentPosition;
		}

		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			currentPosition++; // iterator returned by end points behind to list so we have to increase before
			if (currentPosition == list.end()) {
				return nullptr;
			}
			return *currentPosition;
		}

		throw LogicException("cannot handle given direction", __FILE__, __LINE__);
	}

	protected: void removeFromList(
		std::list<std::shared_ptr<Model_Rule>>& list,
		std::shared_ptr<Model_Rule> ruleToRemove
	) {
		auto position = std::find(list.begin(), list.end(), ruleToRemove);
		list.erase(position);
	}

	protected: void insertBehind(
		std::list<std::shared_ptr<Model_Rule>>& list,
		std::shared_ptr<Model_Rule> ruleToInsert,
		std::shared_ptr<Model_Rule> position,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		auto insertPosition = std::find(list.begin(), list.end(), position);
		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			insertPosition++;
		}
		list.insert(insertPosition, ruleToInsert);
	}

	protected: std::list<std::shared_ptr<Model_Proxy>> findProxiesWithVisibleToplevelEntries(
		std::list<std::shared_ptr<Model_Proxy>> proxies
	) {
		std::list<std::shared_ptr<Model_Proxy>> result;

		for (auto proxy : proxies) {
			for (auto rule : proxy->rules) {
				if (rule->isVisible) {
					result.push_back(proxy);
					proxy->hasVisibleRules();
					break;
				}
			}
		}

		return result;
	}

	protected: std::shared_ptr<Model_Proxy> getNextProxy(
		std::list<std::shared_ptr<Model_Proxy>> list,
		std::shared_ptr<Model_Proxy> base,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		auto currentPosition = std::find(list.begin(), list.end(), base);

		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::UP) {
			if (currentPosition == list.begin()) {
				return nullptr;
			}
			currentPosition--;
			return *currentPosition;
		}

		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			currentPosition++; // iterator returned by end points behind to list so we have to increase before
			if (currentPosition == list.end()) {
				return nullptr;
			}
			return *currentPosition;
		}

		throw LogicException("cannot handle given direction", __FILE__, __LINE__);
	}

	protected: std::shared_ptr<Model_Rule> getFirstVisibleRule(
		std::shared_ptr<Model_Proxy> proxy,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		auto visibleRules = this->findVisibleRules(proxy->rules, nullptr);
		if (visibleRules.size() == 0) {
			return nullptr;
		}
		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::UP) {
			return visibleRules.back();
		}

		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			return visibleRules.front();
		}

		throw LogicException("cannot handle given direction", __FILE__, __LINE__);
	}

	protected: void insertIntoSubmenu(
		std::shared_ptr<Model_Rule>& submenu,
		std::shared_ptr<Model_Rule> ruleToInsert,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			submenu->subRules.push_front(ruleToInsert);
		}

		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::UP) {
			submenu->subRules.push_back(ruleToInsert);
		}
	}

	protected: unsigned int countVisibleRulesOnToplevel(std::shared_ptr<Model_Proxy> proxy)
	{
		unsigned int count = 0;

		for (auto rule : proxy->rules) {
			if (rule->isVisible) {
				count++;
			}
		}

		return count;
	}

	protected: Controller_Helper_RuleMover_AbstractStrategy::Direction flipDirection(
		Controller_Helper_RuleMover_AbstractStrategy::Direction in
	) {
		if (in == Controller_Helper_RuleMover_AbstractStrategy::Direction::UP) {
			return Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN;
		}

		if (in == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			return Controller_Helper_RuleMover_AbstractStrategy::Direction::UP;
		}

		throw LogicException("cannot handle given direction", __FILE__, __LINE__);
	}

	protected: void moveRuleToOtherProxy(
		std::shared_ptr<Model_Rule> ruleToMove,
		std::shared_ptr<Model_Proxy> sourceProxy,
		std::shared_ptr<Model_Proxy> destination,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		// replace ruleToMove by an invisible copy
		auto ruleToMoveSource = std::find(sourceProxy->rules.begin(), sourceProxy->rules.end(), ruleToMove);
		auto dummyRule = ruleToMove->clone();
		dummyRule->setVisibility(false);
		*ruleToMoveSource = dummyRule;

		this->insertIntoProxy(ruleToMove, destination, direction);
	}

	protected: void insertIntoProxy(
		std::shared_ptr<Model_Rule> ruleToInsert,
		std::shared_ptr<Model_Proxy> destination,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		// remove old equivalent rule
		destination->removeEquivalentRules(ruleToInsert);

		// do the insertion
		auto insertPosition = destination->rules.begin();
		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::UP) {
			insertPosition = destination->rules.end();
		}

		destination->rules.insert(insertPosition, ruleToInsert);
	}

	/**
	 * extended version with replacement of old rule
	 */
	protected: void insertAsNewProxy(
		std::shared_ptr<Model_Rule> ruleToMove,
		std::shared_ptr<Model_Proxy> proxyToCopy,
		std::shared_ptr<Model_Proxy> destination,
		std::shared_ptr<Model_ListCfg> listCfg,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction,
		bool reAddOldRuleInvisible = true
	) {
		// replace existing rule on old proxy with invisible copy
		auto oldPos = std::find(proxyToCopy->rules.begin(), proxyToCopy->rules.end(), ruleToMove);
		auto ruleCopy = ruleToMove->clone();
		ruleCopy->setVisibility(false);
		*oldPos = ruleCopy;

		this->insertAsNewProxy(ruleToMove, proxyToCopy->dataSource, destination, listCfg, direction);
	}

	protected: void insertAsNewProxy(
		std::shared_ptr<Model_Rule> ruleToMove,
		std::shared_ptr<Model_Script> sourceScript,
		std::shared_ptr<Model_Proxy> destination,
		std::shared_ptr<Model_ListCfg> listCfg,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		// prepare new proxy containing ruleToMove as the only visible entry
		auto newProxy = std::make_shared<Model_Proxy>(sourceScript, false);

		newProxy->removeEquivalentRules(ruleToMove);

		switch (direction) {
			case Controller_Helper_RuleMover_AbstractStrategy::Direction::UP:
				newProxy->rules.push_back(ruleToMove);
				break;
			case Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN:
				newProxy->rules.push_front(ruleToMove);
				break;
			default:
				throw LogicException("cannot handle given direction", __FILE__, __LINE__);
		}

		// insert the new proxy
		auto insertPosition = std::find(listCfg->proxies.begin(), listCfg->proxies.end(), destination);
		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			insertPosition++;
		}

		listCfg->proxies.insert(insertPosition, newProxy);

		listCfg->renumerate();
	}
};

#endif
