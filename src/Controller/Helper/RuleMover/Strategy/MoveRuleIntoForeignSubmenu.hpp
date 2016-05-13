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

#ifndef INC_Controller_Helper_RuleMover_Strategy_MoveRuleIntoForeignSubmenu
#define INC_Controller_Helper_RuleMover_Strategy_MoveRuleIntoForeignSubmenu

#include "../../../../Model/Rule.hpp"
#include "../../../../Model/ListCfg.hpp"
#include "../AbstractStrategy.hpp"
#include "../../../../lib/Trait/LoggerAware.hpp"
#include <memory>

class Controller_Helper_RuleMover_Strategy_MoveRuleIntoForeignSubmenu :
	public Controller_Helper_RuleMover_AbstractStrategy,
	public Model_ListCfg_Connection,
	public Trait_LoggerAware
{
	public: Controller_Helper_RuleMover_Strategy_MoveRuleIntoForeignSubmenu()
		: Controller_Helper_RuleMover_AbstractStrategy("MoveRuleIntoForeignSubmenu")
	{}

	public: void move(std::shared_ptr<Model_Rule> rule, Controller_Helper_RuleMover_AbstractStrategy::Direction direction)
	{
		auto proxy = this->grublistCfg->proxies.getProxyByRule(rule);
		auto proxiesWithVisibleEntries = this->findProxiesWithVisibleToplevelEntries(this->grublistCfg->proxies);

		auto nextProxy = this->getNextProxy(proxiesWithVisibleEntries, proxy, direction);
		if (nextProxy == nullptr) {
			throw Controller_Helper_RuleMover_MoveFailedException("need next proxy", __FILE__, __LINE__);
		}

		auto previousProxy = this->getNextProxy(proxiesWithVisibleEntries, proxy, this->flipDirection(direction));

		if (proxy->dataSource == nextProxy->dataSource) {
			throw Controller_Helper_RuleMover_MoveFailedException("next proxy is not a foreign proxy", __FILE__, __LINE__);
		}

		auto firstVisibleRuleOfNextProxy = this->getFirstVisibleRule(nextProxy, direction);

		assert(firstVisibleRuleOfNextProxy != nullptr); // we got the proxy from a list of proxies with visible rules

		if (firstVisibleRuleOfNextProxy->type != Model_Rule::RuleType::SUBMENU) {
			throw Controller_Helper_RuleMover_MoveFailedException("first rule of next proxy is not a submenu", __FILE__, __LINE__);
		}

		// replace old rule with invisible copy
		auto ruleCopy = rule->clone();
		ruleCopy->setVisibility(false);
		auto rulePos = std::find(proxy->rules.begin(), proxy->rules.end(), rule);
		assert(rulePos != proxy->rules.end());
		*rulePos = ruleCopy;

		// insert into submenu of foreign proxy
		this->insertIntoSubmenu(firstVisibleRuleOfNextProxy, rule, direction);

		if (this->countVisibleRulesOnToplevel(proxy) == 0) {
			this->grublistCfg->proxies.deleteProxy(proxy);

			if (previousProxy != nullptr && previousProxy->dataSource == nextProxy->dataSource) {
				this->mergeProxy(previousProxy, nextProxy, direction);
			}
		}
	}

	private: void mergeProxy(
		std::shared_ptr<Model_Proxy> source,
		std::shared_ptr<Model_Proxy> destination,
		Controller_Helper_RuleMover_AbstractStrategy::Direction direction
	) {
		auto list = this->findVisibleRules(source->rules, nullptr);
		if (direction == Controller_Helper_RuleMover_AbstractStrategy::Direction::DOWN) {
			list.reverse();
		}

		for (auto rule : list) {
			this->moveRuleToOtherProxy(rule, source, destination, direction);
		}

		this->grublistCfg->proxies.deleteProxy(source);
	}
};



#endif /* MOVERULEINTOFOREIGNSUBMENU_HPP_ */
