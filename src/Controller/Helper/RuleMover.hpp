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

#ifndef RULEMOVER_HPP_
#define RULEMOVER_HPP_

#include "../../Model/Rule.hpp"
#include "../../Model/ListCfg.hpp"
#include "RuleMover/AbstractStrategy.hpp"
#include "RuleMover/MoveFailedException.hpp"
#include <memory>

class Controller_Helper_RuleMover :
	public Model_ListCfg_Connection,
	public Trait_LoggerAware
{
	private: std::list<std::shared_ptr<Controller_Helper_RuleMover_AbstractStrategy>> strategies;

	public: void move(std::shared_ptr<Model_Rule> rule, Controller_Helper_RuleMover_AbstractStrategy::Direction direction)
	{
		assert(this->grublistCfg != nullptr);

		for (auto strategy : this->strategies) {
			try {
				this->log("trying move strategy \"" + strategy->getName() + "\"", Logger::INFO);
				strategy->move(rule, direction);
				this->log("move strategy \"" + strategy->getName() + "\" was successful", Logger::INFO);
				return;
			} catch (Controller_Helper_RuleMover_MoveFailedException const& e) {
				continue;
			}
		}
		throw NoMoveTargetException("cannot move this rule. No successful strategy found", __FILE__, __LINE__);
	}

	public: void addStrategy(std::shared_ptr<Controller_Helper_RuleMover_AbstractStrategy> strategy)
	{
		this->strategies.push_back(strategy);
	}
};

class Controller_Helper_RuleMover_Connection
{
	protected: std::shared_ptr<Controller_Helper_RuleMover> ruleMover;

	public:	virtual ~Controller_Helper_RuleMover_Connection(){}

	public: void setRuleMover(std::shared_ptr<Controller_Helper_RuleMover> ruleMover)
	{
		this->ruleMover = ruleMover;
	}
};

#endif /* RULEMOVER_HPP_ */
