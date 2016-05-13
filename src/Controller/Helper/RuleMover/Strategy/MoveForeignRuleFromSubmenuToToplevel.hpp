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

#ifndef INC_Controller_Helper_RuleMover_Strategy_MoveForeignRuleFromSubmenuToToplevel
#define INC_Controller_Helper_RuleMover_Strategy_MoveForeignRuleFromSubmenuToToplevel

#include "../../../../Model/Rule.hpp"
#include "../../../../Model/ListCfg.hpp"
#include "../AbstractStrategy.hpp"
#include "../../../../lib/Trait/LoggerAware.hpp"
#include <memory>

class Controller_Helper_RuleMover_Strategy_MoveForeignRuleFromSubmenuToToplevel :
	public Controller_Helper_RuleMover_AbstractStrategy,
	public Model_ListCfg_Connection,
	public Trait_LoggerAware
{
	public: Controller_Helper_RuleMover_Strategy_MoveForeignRuleFromSubmenuToToplevel()
		: Controller_Helper_RuleMover_AbstractStrategy("MoveForeignRuleFromSubmenuToToplevel")
	{}

	public: void move(std::shared_ptr<Model_Rule> rule, Controller_Helper_RuleMover_AbstractStrategy::Direction direction)
	{
		if (rule->dataSource == nullptr) {
			throw Controller_Helper_RuleMover_MoveFailedException("rule must have a dataSource", __FILE__, __LINE__);
		}

		auto proxy = this->grublistCfg->proxies.getProxyByRule(rule);

		auto ownScript = this->grublistCfg->repository.getScriptByEntry(rule->dataSource);

		if (ownScript == proxy->dataSource) {
			throw Controller_Helper_RuleMover_MoveFailedException("rule is not a foreign rule", __FILE__, __LINE__);
		}

		auto parentRule = proxy->getParentRule(rule);
		assert(parentRule != nullptr);

		auto& sourceRuleList = proxy->getRuleList(parentRule);

		auto parentOfParent = proxy->getParentRule(parentRule);

		if (parentOfParent != nullptr) {
			throw Controller_Helper_RuleMover_MoveFailedException("destination is another submenu", __FILE__, __LINE__);
		}

		auto& ruleList = proxy->getRuleList(parentOfParent);
		auto visibleRules = this->findVisibleRules(ruleList, rule);

		auto nextRule = this->getNextRule(visibleRules, parentRule, direction);

		auto proxiesWithVisibleEntries = this->findProxiesWithVisibleToplevelEntries(this->grublistCfg->proxies);
		auto nextProxy = this->getNextProxy(proxiesWithVisibleEntries, proxy, direction);

		this->removeFromList(sourceRuleList, rule);

		if (nextRule != nullptr) {
			// we are not at the end of current proxy: split current proxy and insert new proxy
			this->splitProxyAndInsertBetween(rule, ownScript, this->grublistCfg->proxies, proxy, parentRule, direction);
		} else if (nextProxy->dataSource == ownScript) {
			// next proxy is from same script: move rule into nextProxy
			this->insertIntoProxy(rule, nextProxy, direction);
		} else {
			// next proxy is from different script: insert a new proxy before
			this->insertAsNewProxy(rule, ownScript, proxy, this->grublistCfg, direction);
		}

		if (sourceRuleList.size() == 0) {
			this->removeFromList(ruleList, parentRule);
		}
	}

	private: void splitProxyAndInsertBetween(
		std::shared_ptr<Model_Rule> ruleToInsert,
		std::shared_ptr<Model_Script> scriptOfRuleToInsert,
		std::list<std::shared_ptr<Model_Proxy>>& proxyList,
		std::shared_ptr<Model_Proxy> proxyToSplit,
		std::shared_ptr<Model_Rule> position,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		//auto splitPos = proxy
		auto newProxy = std::make_shared<Model_Proxy>(proxyToSplit->dataSource, false);

		bool isAfterPosition = false;
		for (auto rule : proxyToSplit->rules) {
			if (rule == position) {
				isAfterPosition = true;
				continue;
			}

			auto loopDirection = Controller_Helper_RuleMover_AbstractStrategy::Direction::UP;

			if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::UP && !isAfterPosition) {
				this->moveRuleToOtherProxy(rule, proxyToSplit, newProxy, loopDirection);
			}

			if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN && isAfterPosition) {
				this->moveRuleToOtherProxy(rule, proxyToSplit, newProxy, loopDirection);
			}
		}

		auto newProxyPosition = std::find(proxyList.begin(), proxyList.end(), proxyToSplit);
		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			newProxyPosition++;
		}

		proxyList.insert(newProxyPosition, newProxy);

		this->insertAsNewProxy(ruleToInsert, scriptOfRuleToInsert, proxyToSplit, this->grublistCfg, direction);
	}
};

#endif
