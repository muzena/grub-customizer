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

#ifndef SETTINGSCONTROLLERIMPL_H_
#define SETTINGSCONTROLLERIMPL_H_

#include "../Model/ListCfg.hpp"
#include <libintl.h>
#include <locale.h>
#include <sstream>
#include "../config.hpp"

#include "../Model/Env.hpp"

#include "../Model/MountTable.hpp"

#include "../Model/Installer.hpp"

#include "../Model/ListCfg.hpp"
#include "../View/Settings.hpp"
#include "../View/Trait/ViewAware.hpp"
#include "../Model/FbResolutionsGetter.hpp"
#include "../Model/DeviceDataList.hpp"
#include "../lib/ContentParserFactory.hpp"
#include "../Mapper/EntryName.hpp"

#include "Common/ControllerAbstract.hpp"

#include "../lib/Trait/LoggerAware.hpp"

#include "../lib/Exception.hpp"
#include "Helper/Thread.hpp"

class SettingsController :
	public Controller_Common_ControllerAbstract,
	public View_Trait_ViewAware<View_Settings>,
	public Model_ListCfg_Connection,
	public Model_SettingsManagerData_Connection,
	public Model_FbResolutionsGetter_Connection,
	public Model_Env_Connection,
	public Controller_Helper_Thread_Connection,
	public Bootstrap_Application_Object_Connection
{
	private: bool syncActive; // should only be controlled by syncSettings()

	public:	Model_FbResolutionsGetter& getFbResolutionsGetter()
	{
		return *this->fbResolutionsGetter;
	}


	public: void showSettingsDlg()
	{
		this->view->show();
	}

	public: SettingsController() :
		Controller_Common_ControllerAbstract("settings"),
		syncActive(false)
	{
	}

	public: void initViewEvents() override
	{
		using namespace std::placeholders;

		this->view->onDefaultSystemChange = std::bind(std::mem_fn(&SettingsController::updateDefaultSystemAction), this);
		this->view->onCustomSettingChange = std::bind(std::mem_fn(&SettingsController::updateCustomSettingAction), this, _1);
		this->view->onAddCustomSettingClick = std::bind(std::mem_fn(&SettingsController::addCustomSettingAction), this);
		this->view->onRemoveCustomSettingClick = std::bind(std::mem_fn(&SettingsController::removeCustomSettingAction), this, _1);
		this->view->onShowMenuSettingChange = std::bind(std::mem_fn(&SettingsController::updateShowMenuSettingAction), this);
		this->view->onOsProberSettingChange = std::bind(std::mem_fn(&SettingsController::updateOsProberSettingAction), this);
		this->view->onTimeoutSettingChange = std::bind(std::mem_fn(&SettingsController::updateTimeoutSettingAction), this);
		this->view->onKernelParamsChange = std::bind(std::mem_fn(&SettingsController::updateKernelParamsAction), this);
		this->view->onRecoverySettingChange = std::bind(std::mem_fn(&SettingsController::updateRecoverySettingAction), this);
		this->view->onCustomResolutionChange = std::bind(std::mem_fn(&SettingsController::updateCustomResolutionAction), this);
		this->view->onUseCustomResolutionChange = std::bind(std::mem_fn(&SettingsController::updateUseCustomResolutionAction), this);
		this->view->onHide = std::bind(std::mem_fn(&SettingsController::hideAction), this);
	}

	public: void initFbResolutionsGetterEvents() override
	{
		this->fbResolutionsGetter->onFinish = std::bind(std::mem_fn(&SettingsController::updateResolutionlistThreadedAction), this);
	}

	public: void initApplicationEvents() override
	{
		this->applicationObject->onSettingsShowRequest.addHandler(std::bind(std::mem_fn(&SettingsController::showAction), this));
		this->applicationObject->onEnvChange.addHandler(std::bind(std::mem_fn(&SettingsController::hideAction), this));

		this->applicationObject->onInit.addHandler(
			[this] () {
				//loading the framebuffer resolutions in background…
				this->log("Loading Framebuffer resolutions (background process)", Logger::EVENT);
				this->threadHelper->runAsThread(std::bind(std::mem_fn(&SettingsController::loadResolutionsAction), this));
			}
		);

		this->applicationObject->onListModelChange.addHandler(std::bind(std::mem_fn(&SettingsController::updateSettingsDataAction), this));

		this->applicationObject->onSettingModelChange.addHandler(std::bind(std::mem_fn(&SettingsController::syncAction), this));
	}

	public: void updateSettingsDataAction()
	{
		this->logActionBegin("update-settings-data");
		try {
			std::list<Model_Proxylist_Item> entryTitles = this->grublistCfg->proxies.generateEntryTitleList();
	
			this->view->clearDefaultEntryChooser();
			this->view->addEntryToDefaultEntryChooser("0", "");
			for (std::list<Model_Proxylist_Item>::iterator iter = entryTitles.begin(); iter != entryTitles.end(); iter++) {
				this->view->addEntryToDefaultEntryChooser(iter->labelPathValue, iter->labelPathLabel);
			}
			this->view->addEntryToDefaultEntryChooser("", "");
	
			this->syncSettings();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void loadResolutionsAction()
	{
		this->logActionBegin("load-resolutions");
		try {
			this->fbResolutionsGetter->load();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}


	public: void updateResolutionlistAction()
	{
		this->logActionBegin("update-resolutionlist");
		try {
			const std::list<std::string>& data = this->fbResolutionsGetter->getData();
			this->view->clearResolutionChooser();
			for (std::list<std::string>::const_iterator iter = data.begin(); iter != data.end(); iter++) {
				this->view->addResolution(*iter);
			}
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateResolutionlistThreadedAction()
	{
		this->logActionBeginThreaded("update-resolutionlist-threaded");
		try {
			this->threadHelper->runDispatched(std::bind(std::mem_fn(&SettingsController::updateResolutionlistAction), this));
		} catch (Exception const& e) {
			this->applicationObject->onThreadError.exec(e);
		}
		this->logActionEndThreaded();
	}

	public: void updateDefaultSystemAction()
	{
		this->logActionBegin("update-default-system");
		try {
			if (this->view->getActiveDefEntryOption() == View_Settings::DEF_ENTRY_SAVED){
				this->settings->setValue("GRUB_DEFAULT", "saved");
				this->settings->setValue("GRUB_SAVEDEFAULT", "true");
				this->settings->setIsActive("GRUB_SAVEDEFAULT", true);
			}
			else {
				this->settings->setValue("GRUB_DEFAULT", this->view->getSelectedDefaultGrubValue());
				this->settings->setValue("GRUB_SAVEDEFAULT", "false");
			}
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateCustomSettingAction(std::string const& name)
	{
		this->logActionBegin("update-custom-setting");
		try {
			View_Settings::CustomOption c = this->view->getCustomOption(name);
			this->settings->renameItem(c.old_name, c.name);
			this->settings->setValue(c.name, c.value);
			this->settings->setIsActive(c.name, c.isActive);
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void addCustomSettingAction()
	{
		this->logActionBegin("add-custom-setting");
		try {
			std::string newSettingName = this->settings->addNewItem();
			this->syncSettings();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void removeCustomSettingAction(std::string const& name)
	{
		this->logActionBegin("remove-custom-setting");
		try {
			this->settings->removeItem(name);
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateShowMenuSettingAction()
	{
		this->logActionBegin("update-show-menu-setting");
		try {
			std::string val = this->settings->getValue("GRUB_HIDDEN_TIMEOUT");
			if (val == "" || val.find_first_not_of("0123456789") != -1) {
				this->settings->setValue("GRUB_HIDDEN_TIMEOUT", "0"); //create this entry - if it has an invalid value
			}
			this->settings->setIsActive("GRUB_HIDDEN_TIMEOUT", !this->view->getShowMenuCheckboxState());
			if (!this->view->getShowMenuCheckboxState() && this->view->getOsProberCheckboxState()){
				this->view->showHiddenMenuOsProberConflictMessage();
			}
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateOsProberSettingAction()
	{
		this->logActionBegin("update-os-prober-setting");
		try {
			this->settings->setValue("GRUB_DISABLE_OS_PROBER", this->view->getOsProberCheckboxState() ? "false" : "true");
			this->settings->setIsActive("GRUB_DISABLE_OS_PROBER", !this->view->getOsProberCheckboxState());
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateTimeoutSettingAction()
	{
		this->logActionBegin("update-timeout-setting");
		try {
			std::string timeoutValue = this->view->getTimeoutValueString();
			if (!this->view->getTimeoutActive() && timeoutValue != "-1") {
				timeoutValue = "-1";
			}
	
			if (this->view->getShowMenuCheckboxState()){
				this->settings->setValue("GRUB_TIMEOUT", timeoutValue);
			}
			else {
				this->settings->setValue("GRUB_HIDDEN_TIMEOUT", this->view->getTimeoutValueString());
			}
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateKernelParamsAction()
	{
		this->logActionBegin("update-kernel-params");
		try {
			this->settings->setValue("GRUB_CMDLINE_LINUX_DEFAULT", this->view->getKernelParams());
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateRecoverySettingAction()
	{
		this->logActionBegin("update-recovery-setting");
		try {
			// GRUB_DISABLE_LINUX_RECOVERY is used until GRUB 1.98
			if (this->settings->getValue("GRUB_DISABLE_LINUX_RECOVERY") != "true") {
				this->settings->setValue("GRUB_DISABLE_LINUX_RECOVERY", "true");
			}
			this->settings->setIsActive("GRUB_DISABLE_LINUX_RECOVERY", !this->view->getRecoveryCheckboxState());
	
			// GRUB_DISABLE_RECOVERY is the new version
			if (this->settings->getValue("GRUB_DISABLE_RECOVERY") != "true") {
				this->settings->setValue("GRUB_DISABLE_RECOVERY", "true");
			}
			this->settings->setIsActive("GRUB_DISABLE_RECOVERY", !this->view->getRecoveryCheckboxState());
	
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateCustomResolutionAction()
	{
		this->logActionBegin("update-custom-resolution");
		try {
			this->settings->setValue("GRUB_GFXMODE", this->view->getResolution());
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateUseCustomResolutionAction()
	{
		this->logActionBegin("update-use-custom-resolution");
		try {
			if (this->settings->getValue("GRUB_GFXMODE") == "") {
				this->settings->setValue("GRUB_GFXMODE", "saved"); //use saved as default value (if empty)
			}
			this->settings->setIsActive("GRUB_GFXMODE", this->view->getResolutionCheckboxState());
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void hideAction()
	{
		this->logActionBegin("hide");
		try {
			this->view->hide();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void showAction()
	{
		this->logActionBegin("show");
		try {
			this->view->show();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void syncAction()
	{
		this->logActionBegin("sync");
		try {
			this->syncSettings();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	private: void syncSettings()
	{
		if (this->syncActive) {
			return;
		}
		this->syncActive = true;
		std::string sel = this->view->getSelectedCustomOption();
		this->view->removeAllSettingRows();
		for (std::list<Model_SettingsStore_Row>::iterator iter = this->settings->begin(); iter != this->settings->end(); this->settings->iter_to_next_setting(iter)){
			this->view->addCustomOption(iter->isActive, iter->name, iter->value);
		}
		this->view->selectCustomOption(sel);
		std::string defEntry = this->settings->getValue("GRUB_DEFAULT");
		if (defEntry == "saved"){
			this->view->setActiveDefEntryOption(View_Settings::DEF_ENTRY_SAVED);
		}
		else {
			this->view->setActiveDefEntryOption(View_Settings::DEF_ENTRY_PREDEFINED);
			this->view->setDefEntry(defEntry);
		}

		this->view->setShowMenuCheckboxState(!this->settings->isActive("GRUB_HIDDEN_TIMEOUT", true));
		this->view->setOsProberCheckboxState(!this->settings->isActive("GRUB_DISABLE_OS_PROBER", true));

		std::string timeoutStr;
		if (this->view->getShowMenuCheckboxState())
			timeoutStr = this->settings->getValue("GRUB_TIMEOUT");
		else
			timeoutStr = this->settings->getValue("GRUB_HIDDEN_TIMEOUT");

		if (timeoutStr == "" || (timeoutStr.find_first_not_of("0123456789") != -1 && timeoutStr != "-1")) {
			timeoutStr = "10"; //default value
		}
		std::istringstream in(timeoutStr);
		int timeout;
		in >> timeout;
		this->view->setTimeoutValue(timeout == -1 ? 10 : timeout);
		this->view->setTimeoutActive(timeout != -1);

		this->view->setKernelParams(this->settings->getValue("GRUB_CMDLINE_LINUX_DEFAULT"));
		this->view->setRecoveryCheckboxState(!this->settings->isActive("GRUB_DISABLE_RECOVERY", true));

		this->view->setResolutionCheckboxState(this->settings->isActive("GRUB_GFXMODE", true));
		this->view->setResolution(this->settings->getValue("GRUB_GFXMODE"));

		if (this->settings->reloadRequired()) {
			this->applicationObject->onListRelevantSettingChange.exec();
		}
		this->applicationObject->onSettingModelChange.exec();
		this->syncActive = false;
	}
};

#endif
