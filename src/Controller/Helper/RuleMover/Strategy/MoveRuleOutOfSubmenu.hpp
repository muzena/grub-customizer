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

#ifndef INC_Controller_Helper_RuleMover_Strategy_MoveRuleOutOfSubmenu
#define INC_Controller_Helper_RuleMover_Strategy_MoveRuleOutOfSubmenu

#include "../../../../Model/Rule.hpp"
#include "../../../../Model/ListCfg.hpp"
#include "../AbstractStrategy.hpp"
#include "../../../../lib/Trait/LoggerAware.hpp"
#include <memory>

class Controller_Helper_RuleMover_Strategy_MoveRuleOutOfSubmenu :
	public Controller_Helper_RuleMover_AbstractStrategy,
	public Model_ListCfg_Connection,
	public Trait_LoggerAware
{
	public: Controller_Helper_RuleMover_Strategy_MoveRuleOutOfSubmenu()
		: Controller_Helper_RuleMover_AbstractStrategy("MoveRuleOutOfSubmenu")
	{}

	public: void move(std::shared_ptr<Model_Rule> rule, Controller_Helper_RuleMover_AbstractStrategy::Direction direction)
	{
		auto proxy = this->grublistCfg->proxies.getProxyByRule(rule);
		auto parentRule = proxy->getParentRule(rule);

		if (parentRule == nullptr) {
			throw Controller_Helper_RuleMover_MoveFailedException(
				"having no parent rule - so we already are on toplevel and cannot move out", __FILE__, __LINE__
			);
		}

		auto& ruleList = proxy->getRuleList(parentRule);

		auto visibleRules = this->findVisibleRules(ruleList, rule);

		auto nextRule = this->getNextRule(visibleRules, rule, direction);

		auto& destinationRuleList = proxy->getRuleList(proxy->getParentRule(parentRule));

		this->removeFromList(ruleList, rule);
		this->insertBehind(destinationRuleList, rule, parentRule, direction);

		if (ruleList.size() == 0) {
			this->removeFromList(destinationRuleList, parentRule);
		}
	}
};
#endif
