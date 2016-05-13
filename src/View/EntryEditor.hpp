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

#ifndef ENTRYEDITDLG_H_
#define ENTRYEDITDLG_H_
#include <string>
#include <map>
#include <list>
#include <functional>

#include "../lib/Type.hpp"
#include "../lib/Trait/LoggerAware.hpp"
#include "../Model/DeviceDataListInterface.hpp"

class View_EntryEditor :
	public Trait_LoggerAware,
	public Model_DeviceDataListInterface_Connection
{
public:
	std::function<void ()> onApplyClick;
	std::function<void ()> onSourceModification;
	std::function<void ()> onOptionModification;
	std::function<void (std::string const& newType)> onTypeSwitch;
	std::function<void (std::string, std::string, std::list<std::string>)> onFileChooserSelection;
	std::function<void ()> onNameChange;

	virtual inline ~View_EntryEditor() {};

	virtual void show() = 0;
	virtual void setSourcecode(std::string const& source) = 0;
	virtual void showSourceBuildError() = 0;
	virtual void setApplyEnabled(bool value) = 0;
	virtual std::string getSourcecode() = 0;

	virtual void addOption(std::string const& name, std::string const& value) = 0;
	virtual void setOptions(std::map<std::string, std::string> options) = 0;
	virtual std::map<std::string, std::string> getOptions() const = 0;
	virtual void removeOptions() = 0;

	virtual void setRulePtr(Rule* rulePtr) = 0;
	virtual Rule* getRulePtr() = 0;

	virtual void hide() = 0;

	virtual void setAvailableEntryTypes(std::list<std::string> const& names) = 0;
	virtual void selectType(std::string const& name) = 0;
	virtual std::string getSelectedType() const = 0;
	virtual void setName(std::string const& name) = 0;
	virtual std::string getName() = 0;
	virtual void setNameFieldVisibility(bool visible) = 0;

	virtual void setErrors(std::list<std::string> const& errors) = 0;

	virtual void setNameIsValid(bool valid) = 0;
	virtual void setTypeIsValid(bool valid) = 0;
};

#endif /* ENTRYEDITDLG_H_ */
