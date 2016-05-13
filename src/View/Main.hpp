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

#ifndef GRUBLISTCFGDLG_H_
#define GRUBLISTCFGDLG_H_

#include <string>
#include <vector>
#include <map>
#include <list>

#include "../lib/Type.hpp"
#include "../lib/Trait/LoggerAware.hpp"
#include "Model/ListItem.hpp"
#include <functional>

/**
 * Interface for dialogs which lets the user control the grub list
 */
class View_Main : public Trait_LoggerAware {
public:
	virtual inline ~View_Main() {};

	std::function<void (std::list<Rule*> rules, bool force)> onRemoveRulesClick;
	std::function<void ()> onShowSettingsClick;
	std::function<void ()> onReloadClick;
	std::function<void ()> onSaveClick;
	std::function<void ()> onShowEnvEditorClick;
	std::function<void ()> onShowInstallerClick;
	std::function<void (std::list<Rule*> childItems)> onCreateSubmenuClick;
	std::function<void (std::list<Rule*> childItems)> onRemoveSubmenuClick;
	std::function<void (Rule* rule)> onShowEntryEditorClick;
	std::function<void ()> onShowEntryCreatorClick;
	std::function<void ()> onShowAboutClick;
	std::function<void ()> onExitClick;
	std::function<void (Rule* entry, std::string const& newText)> onRenameClick;
	std::function<void ()> onRevertClick;
	std::function<void (std::list<Rule*> rules, int direction)> onMoveClick;
	std::function<void ()> onCancelBurgSwitcherClick;
	std::function<void (bool burgChosen)> onInitModeClick;
	std::function<void (Rule* rule, bool startEdit)> onRuleSelection;
	std::function<void (unsigned int pos)> onTabChange;
	std::function<void (ViewOption option, bool value)> onViewOptionChange;
	std::function<void (Rule* entry, bool state)> onEntryStateChange;
	std::function<void (std::list<Rule*> selectedRules)> onSelectionChange;


	//show this dialog without waiting
	virtual void show()=0;
	//hide this dialog
	virtual void hide() = 0;
	//show this dialog and wait until the window has been closed
	virtual void run()=0;
	//hide this window and close the whole application
	virtual void close()=0;
	//show the dialog which lets the user choose burg or grub
	virtual void showBurgSwitcher()=0;
	//hide the dialog which lets the user choose burg or grub
	virtual void hideBurgSwitcher()=0;
	//returns whether the list configuration window is visible at the moment
	virtual bool isVisible()=0;

	//notifies the window about which mode is used (grub<>burg)
	virtual void setIsBurgMode(bool isBurgMode)=0;
	//determines what users should be able to do and what not
	virtual void setLockState(int state)=0;
	virtual void updateLockState() = 0;

	//set the progress of the actual action (loading/saving) to be showed as progress bar for example
	virtual void setProgress(double progress)=0;
	//pulse the progress
	virtual void progress_pulse()=0;
	//hide the progress bar, will be executed after loading has been completed
	virtual void hideProgressBar()=0;
	//sets the text to be showed inside the status bar
	virtual void setStatusText(std::string const& new_status_text)=0;
	virtual void setStatusText(std::string const& name, int pos, int max)=0;
	//add entry to the end of the last script of the list
	virtual void appendEntry(View_Model_ListItem<Rule, Proxy> const& listItem)=0;
	//notifies the user about the problem that no grublistcfg_proxy has been found
	virtual void showProxyNotFoundMessage()=0;
	//creates a string for an other entry placeholder
	virtual std::string createNewEntriesPlaceholderString(std::string const& parentMenu)=0;
	//creates the string for plaintexts
	virtual std::string createPlaintextString() const=0;

	//asks the user if he wants to exit the whole application
	virtual int showExitConfirmDialog(int type)=0;
	//show the given error message
	virtual void showErrorMessage(std::string const& msg)=0;

	virtual void showConfigSavingError(std::string const& message) = 0;

	//shows an error message including an option for changing the environment
	virtual bool askForEnvironmentSettings(std::string const& failedCmd, std::string const& errorMessage) = 0;
	//remove everything from the list
	virtual void clear()=0;

	//asks the user whether the current config should be dropped while another action is started
	virtual bool confirmUnsavedSwitch() = 0;

	//assigns a new name to the rule item
	virtual void setRuleName(Rule* rule, std::string const& newName)=0;

	//select the given rule
	virtual void selectRule(Rule* rule, bool startEdit = false)=0;

	// select multiple rules
	virtual void selectRules(std::list<Rule*> rules)=0;

	// set whether the trash pane should be visible
	virtual void setTrashPaneVisibility(bool value) = 0;

	// show the warning that config has changed to propose a reload
	virtual void showReloadRecommendation() = 0;

	// hide the warning that config has changed to propose a reload
	virtual void hideReloadRecommendation() = 0;

	virtual void showPlaintextRemoveWarning() = 0;

	virtual void showScriptUpdateInfo() = 0;
	virtual void hideScriptUpdateInfo() = 0;

	virtual void showSystemRuleRemoveWarning() = 0;

	virtual void setOption(ViewOption option, bool value) = 0;

	virtual std::map<ViewOption, bool> const& getOptions() = 0;
	virtual void setOptions(std::map<ViewOption, bool> const& options) = 0;

	virtual void setEntryVisibility(Rule* entry, bool value) = 0;
};

#endif
