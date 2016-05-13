/*
 * Copyright (C) 2010-2014 Daniel Richter <danielrichter2007@web.de>
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

#ifndef SRC_BOOTSTRAP_FACTORY_HPP_
#define SRC_BOOTSTRAP_FACTORY_HPP_

#include "../View/Trait/ViewAware.hpp"
#include "../Model/Env.hpp"
#include "../Model/ListCfg.hpp"
#include "../Model/MountTable.hpp"
#include "../Model/SettingsManagerData.hpp"
#include "../Model/Installer.hpp"
#include "../Model/FbResolutionsGetter.hpp"
#include "../Model/DeviceDataList.hpp"
#include "../lib/ContentParser/FactoryImpl.hpp"
#include "../Mapper/EntryNameImpl.hpp"
#include "../Model/ThemeManager.hpp"
#include "../Model/DeviceMap.hpp"
#include "../Controller/Helper/Thread.hpp"
#include "../Controller/Helper/RuleMover.hpp"
#include "Application.hpp"

class Bootstrap_Factory
{
	public: std::shared_ptr<Model_Env> env;
	public: std::shared_ptr<Model_ListCfg> listcfg;
	public: std::shared_ptr<Model_SettingsManagerData> settings;
	public: std::shared_ptr<Model_Installer> installer;
	public: std::shared_ptr<Model_MountTable> mountTable;
	public: std::shared_ptr<Model_FbResolutionsGetter> fbResolutionsGetter;
	public: std::shared_ptr<Model_DeviceDataList> deviceDataList;
	public: std::shared_ptr<ContentParser_FactoryImpl> contentParserFactory;
	public: std::shared_ptr<Mapper_EntryNameImpl> entryNameMapper;
	public: std::shared_ptr<Model_ThemeManager> themeManager;
	public: std::shared_ptr<Model_DeviceMap> deviceMap;
	public: std::shared_ptr<Controller_Helper_RuleMover> ruleMover;
	public: std::shared_ptr<Logger> logger;

	public: std::shared_ptr<Regex> regexEngine;
	public: std::shared_ptr<Controller_Helper_Thread> threadHelper;

	public: std::shared_ptr<Bootstrap_Application_Object> applicationObject;

	public: Bootstrap_Factory(std::shared_ptr<Bootstrap_Application_Object> applicationObject, std::shared_ptr<Logger> logger)
	{
		this->applicationObject    = applicationObject;
		this->logger               = logger;

		this->regexEngine          = this->createRegexExgine();
		this->threadHelper         = this->createThreadHelper();

		this->env                  = this->create<Model_Env>();
		this->listcfg              = this->create<Model_ListCfg>();
		this->settings             = this->create<Model_SettingsManagerData>();
		this->installer            = this->create<Model_Installer>();
		this->mountTable           = this->create<Model_MountTable>();
		this->fbResolutionsGetter  = this->create<Model_FbResolutionsGetter>();
		this->deviceDataList       = this->create<Model_DeviceDataList>();
		this->contentParserFactory = this->create<ContentParser_FactoryImpl>();
		this->entryNameMapper      = this->create<Mapper_EntryNameImpl>();
		this->themeManager         = this->create<Model_ThemeManager>();
		this->deviceMap            = this->create<Model_DeviceMap>();
		this->ruleMover            = this->create<Controller_Helper_RuleMover>();

		this->bootstrap(this->regexEngine);
		this->bootstrap(this->threadHelper);
	}

	public: template <typename TController, typename TView> std::shared_ptr<TController> createController(std::shared_ptr<TView> view)
	{
		auto controller = std::make_shared<TController>();

		controller->setApplicationObject(this->applicationObject);
		controller->setView(view);

		this->bootstrap(controller);
		this->bootstrap(view);

		return controller;
	}

	public: template <typename T> std::shared_ptr<T> create()
	{
		auto obj = std::make_shared<T>();
		this->bootstrap(obj);
		return obj;
	}

	public: template <typename T> void bootstrap(std::shared_ptr<T> obj)
	{
		{
			std::shared_ptr<Model_Env_Connection> objc = std::dynamic_pointer_cast<Model_Env_Connection>(obj);
			if (objc) {assert(this->env); objc->setEnv(this->env);}
		}
		{
			std::shared_ptr<Model_ListCfg_Connection> objc = std::dynamic_pointer_cast<Model_ListCfg_Connection>(obj);
			if (objc) {assert(this->listcfg); objc->setListCfg(this->listcfg);}
		}
		{
			std::shared_ptr<Model_SettingsManagerData_Connection> objc = std::dynamic_pointer_cast<Model_SettingsManagerData_Connection>(obj);
			if (objc) {assert(this->settings); objc->setSettingsManager(this->settings);}
		}
		{
			std::shared_ptr<Model_Installer_Connection> objc = std::dynamic_pointer_cast<Model_Installer_Connection>(obj);
			if (objc) {assert(this->installer); objc->setInstaller(this->installer);}
		}
		{
			std::shared_ptr<Model_MountTable_Connection> objc = std::dynamic_pointer_cast<Model_MountTable_Connection>(obj);
			if (objc) {assert(this->mountTable); objc->setMountTable(this->mountTable);}
		}
		{
			std::shared_ptr<Model_FbResolutionsGetter_Connection> objc = std::dynamic_pointer_cast<Model_FbResolutionsGetter_Connection>(obj);
			if (objc) {assert(this->fbResolutionsGetter); objc->setFbResolutionsGetter(this->fbResolutionsGetter);}
		}
		{
			std::shared_ptr<Model_DeviceDataList_Connection> objc = std::dynamic_pointer_cast<Model_DeviceDataList_Connection>(obj);
			if (objc) {assert(this->deviceDataList); objc->setDeviceDataList(this->deviceDataList);}
		}
		{
			std::shared_ptr<Model_DeviceDataListInterface_Connection> objc = std::dynamic_pointer_cast<Model_DeviceDataListInterface_Connection>(obj);
			if (objc) {assert(this->deviceDataList); objc->setDeviceDataList(this->deviceDataList);}
		}
		{
			std::shared_ptr<ContentParserFactory_Connection> objc = std::dynamic_pointer_cast<ContentParserFactory_Connection>(obj);
			if (objc) {assert(this->contentParserFactory); objc->setContentParserFactory(this->contentParserFactory);}
		}
		{
			std::shared_ptr<Mapper_EntryName_Connection> objc = std::dynamic_pointer_cast<Mapper_EntryName_Connection>(obj);
			if (objc) {assert(this->entryNameMapper); objc->setEntryNameMapper(this->entryNameMapper);}
		}
		{
			std::shared_ptr<Model_ThemeManager_Connection> objc = std::dynamic_pointer_cast<Model_ThemeManager_Connection>(obj);
			if (objc) {assert(this->themeManager); objc->setThemeManager(this->themeManager);}
		}
		{
			std::shared_ptr<Model_DeviceMap_Connection> objc = std::dynamic_pointer_cast<Model_DeviceMap_Connection>(obj);
			if (objc) {assert(this->deviceMap); objc->setDeviceMap(this->deviceMap);}
		}
		{
			std::shared_ptr<Trait_LoggerAware> objc = std::dynamic_pointer_cast<Trait_LoggerAware>(obj);
			if (objc) {assert(this->logger); objc->setLogger(this->logger);}
		}
		{
			std::shared_ptr<Regex_RegexConnection> objc = std::dynamic_pointer_cast<Regex_RegexConnection>(obj);
			if (objc) {assert(this->regexEngine); objc->setRegexEngine(this->regexEngine);}
		}
		{
			std::shared_ptr<Mutex_Connection> objc = std::dynamic_pointer_cast<Mutex_Connection>(obj);
			if (objc) {objc->setMutex(this->createMutex());}
		}
		{
			std::shared_ptr<Controller_Helper_Thread_Connection> objc = std::dynamic_pointer_cast<Controller_Helper_Thread_Connection>(obj);
			if (objc) {assert(this->threadHelper); objc->setThreadHelper(this->threadHelper);}
		}
		{
			std::shared_ptr<Controller_Helper_RuleMover_Connection> objc = std::dynamic_pointer_cast<Controller_Helper_RuleMover_Connection>(obj);
			if (objc) {assert(this->ruleMover); objc->setRuleMover(this->ruleMover);}
		}
	}

	// external implementations
	private: std::shared_ptr<Regex> createRegexExgine();
	private: std::shared_ptr<Mutex> createMutex();
	private: std::shared_ptr<Controller_Helper_Thread> createThreadHelper();
};

#endif /* SRC_BOOTSTRAP_FACTORY_HPP_ */
