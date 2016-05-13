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

#ifndef TRASHCONTROLLERIMPL_H_
#define TRASHCONTROLLERIMPL_H_

#include "../Model/ListCfg.hpp"
#include "../View/Main.hpp"
#include <libintl.h>
#include <locale.h>
#include <sstream>
#include "../config.hpp"

#include "../Model/Env.hpp"

#include "../Model/MountTable.hpp"

#include "../View/Trash.hpp"

#include "../View/EnvEditor.hpp"
#include "../View/Trait/ViewAware.hpp"
#include "../Mapper/EntryName.hpp"

#include "Common/ControllerAbstract.hpp"

#include "../Model/DeviceDataListInterface.hpp"
#include "../lib/ContentParserFactory.hpp"
#include "Helper/DeviceInfo.hpp"

class TrashController :
	public Controller_Common_ControllerAbstract,
	public View_Trait_ViewAware<View_Trash>,
	public Model_ListCfg_Connection,
	public Mapper_EntryName_Connection,
	public Model_DeviceDataListInterface_Connection,
	public ContentParserFactory_Connection,
	public Model_Env_Connection,
	public Bootstrap_Application_Object_Connection
{
	private: std::list<std::shared_ptr<Model_Rule>> data;

	public:	TrashController() :
		Controller_Common_ControllerAbstract("trash")
	{
	}

	public:	void initViewEvents() override
	{
		using namespace std::placeholders;

		this->view->onRestore = std::bind(std::mem_fn(&TrashController::applyAction), this);
		this->view->onDeleteClick = std::bind(std::mem_fn(&TrashController::deleteCustomEntriesAction), this);
		this->view->onSelectionChange = std::bind(std::mem_fn(&TrashController::updateSelectionAction), this, _1);
	}

	public:	void initApplicationEvents() override
	{
		using namespace std::placeholders;

		this->applicationObject->onEnvChange.addHandler(std::bind(std::mem_fn(&TrashController::hideAction), this));
		this->applicationObject->onListModelChange.addHandler(std::bind(std::mem_fn(&TrashController::updateAction), this));
		this->applicationObject->onEntryRemove.addHandler(std::bind(std::mem_fn(&TrashController::selectEntriesAction), this, _1));
		this->applicationObject->onEntrySelection.addHandler(std::bind(std::mem_fn(&TrashController::selectEntriesAction), this, std::list<Entry*>()));
	}
	
	public:	void updateAction()
	{
		this->logActionBegin("update");
		try {
			this->view->setOptions(this->applicationObject->viewOptions);
			this->refresh();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public:	void applyAction()
	{
		this->logActionBegin("apply");
		try {
			std::list<Rule*> rulePtrs = view->getSelectedEntries();
			this->applicationObject->onEntryInsertionRequest.exec(rulePtrs);
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public:	void hideAction()
	{
		this->logActionBegin("hide");
		try {
			this->view->hide();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void deleteCustomEntriesAction()
	{
		this->logActionBegin("delete-custom-entries");
		try {
			auto deletableEntries = this->view->getSelectedEntries();
			for (auto rulePtr : deletableEntries) {
				assert(this->findRule(rulePtr)->dataSource != nullptr);
				this->grublistCfg->deleteEntry(this->findRule(rulePtr)->dataSource);
			}
			this->refresh();
			this->updateSelectionAction(std::list<Rule*>());
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void selectEntriesAction(std::list<Entry*> const& entries)
	{
		this->logActionBegin("select-entries");
		try {
			// first look for rules in local data, linking to the the given entries
			std::list<Rule*> rules;
			for (auto entryPtr : entries) {
				for (auto rule : this->data) {
					if (entryPtr == rule->dataSource.get()) {
						rules.push_back(rule.get());
					}
				}
			}
			this->view->selectEntries(rules);
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateSelectionAction(std::list<Rule*> const& selectedEntries)
	{
		this->logActionBegin("update-selection");
		try {
			if (selectedEntries.size()) {
				this->applicationObject->onTrashEntrySelection.exec();
				this->view->setRestoreButtonSensitivity(true);
				this->view->setDeleteButtonVisibility(this->ruleListIsDeletable(selectedEntries));
			} else {
				this->view->setRestoreButtonSensitivity(false);
				this->view->setDeleteButtonVisibility(false);
			}
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	private: void refresh()
	{
		assert(this->contentParserFactory != nullptr);
		assert(this->deviceDataList != nullptr);

		this->view->clear();

		this->data = this->grublistCfg->getRemovedEntries();

		this->refreshView(nullptr);
	}

	private: void refreshView(std::shared_ptr<Model_Rule> parent)
	{
		auto& list = parent ? parent->subRules : this->data;
		for (auto rule : list) {
			auto script = rule->dataSource ? this->grublistCfg->repository.getScriptByEntry(rule->dataSource) : nullptr;

			std::string name = rule->outputName;
			if (rule->dataSource && script) {
				name = this->entryNameMapper->map(rule->dataSource, name, rule->type != Model_Rule::SUBMENU);
			}

			View_Model_ListItem<Rule, Script> listItem;
			listItem.name = name;
			listItem.entryPtr = rule.get();
			listItem.scriptPtr = nullptr;
			listItem.is_placeholder = rule->type == Model_Rule::OTHER_ENTRIES_PLACEHOLDER || rule->type == Model_Rule::PLAINTEXT;
			listItem.is_submenu = rule->type == Model_Rule::SUBMENU;
			listItem.scriptName = script ? script->name : "";
			listItem.isVisible = true;
			listItem.parentEntry = parent.get();

			if (rule->dataSource) {
				listItem.options = Controller_Helper_DeviceInfo::fetch(
					rule->dataSource->content,
					*this->contentParserFactory,
					*this->deviceDataList
				);
			}

			this->view->addItem(listItem);

			if (rule->subRules.size()) {
				this->refreshView(rule);
			}
		}
	}

	private: bool ruleListIsDeletable(std::list<Rule*> const& selectedEntries)
	{
		if (selectedEntries.size() == 0) {
			return false;
		}

		for (auto& entry : selectedEntries) {
			if (this->findRule(entry)->type != Model_Rule::NORMAL || this->findRule(entry)->dataSource == nullptr) {
				return false;
			}
			auto script = this->grublistCfg->repository.getScriptByEntry(this->findRule(entry)->dataSource);
			assert(script != nullptr);
			if (!script->isCustomScript) {
				return false;
			}
		}

		return true;
	}

	private: std::shared_ptr<Model_Rule> findRule(Rule* rulePtr, std::shared_ptr<Model_Rule> parent = nullptr)
	{
		std::list<std::shared_ptr<Model_Rule>>& list = parent ? parent->subRules : this->data;

		for (auto rule : list) {
			if (rule.get() == rulePtr) {
				return rule;
			}
			if (rule->subRules.size()) {
				try {
					return this->findRule(rulePtr, rule);
				} catch (ItemNotFoundException const& e) {
					// continue
				}
			}
		}
		throw ItemNotFoundException("rule not found in trash data", __FILE__, __LINE__);
	}
};

#endif
