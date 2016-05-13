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

#include <iostream>

#include "../Bootstrap/View.hpp"
#include "../Bootstrap/Application.hpp"
#include "../Bootstrap/Factory.hpp"
#include "../lib/ContentParser/Chainloader.hpp"
#include "../lib/ContentParser/FactoryImpl.hpp"
#include "../lib/ContentParser/Linux.hpp"
#include "../lib/ContentParser/LinuxIso.hpp"
#include "../lib/ContentParser/Memtest.hpp"
#include "../Controller/Helper/RuleMover/Strategy/MoveRuleOnSameLevelInsideProxy.hpp"
#include "../Controller/Helper/RuleMover/Strategy/MoveRuleIntoSubmenu.hpp"
#include "../Controller/Helper/RuleMover/Strategy/MoveRuleOutOfSubmenu.hpp"
#include "../Controller/Helper/RuleMover/Strategy/MoveRuleOutOfProxyOnToplevel.hpp"
#include "../Controller/Helper/RuleMover/Strategy/MoveRuleIntoForeignSubmenu.hpp"
#include "../Controller/Helper/RuleMover/Strategy/MoveForeignRuleFromSubmenuToToplevel.hpp"
#include "../lib/Logger/Stream.hpp"
#include "../Mapper/EntryNameImpl.hpp"
#include "../config.hpp"
#include "../Controller/AboutController.hpp"
#include "../Controller/EntryEditController.hpp"
#include "../Controller/EnvEditorController.hpp"
#include "../Controller/ErrorController.hpp"
#include "../Controller/InstallerController.hpp"
#include "../Controller/MainController.hpp"
#include "../Controller/SettingsController.hpp"
#include "../Controller/ThemeController.hpp"
#include "../Controller/TrashController.hpp"



int main(int argc, char** argv){
	if (getuid() != 0 && (argc == 1 || argv[1] != std::string("no-fork"))) {
		return system((std::string("pkexec ") + argv[0] + (argc == 2 ? std::string(" ") + argv[1] : "") + " no-fork").c_str());
	}
	setlocale(LC_ALL, "");
	bindtextdomain("grub-customizer", LOCALEDIR);
	textdomain("grub-customizer");

	auto logger = std::make_shared<Logger_Stream>(std::cout);

	Logger::getInstance() = logger;

	try {
		auto application          = std::make_shared<Bootstrap_Application>(argc, argv);
		auto view                 = std::make_shared<Bootstrap_View>();
		auto factory              = std::make_shared<Bootstrap_Factory>(application->applicationObject, logger);

		auto settingsOnDisk       = factory->create<Model_SettingsManagerData>();
		auto savedListCfg         = factory->create<Model_ListCfg>();

		factory->entryNameMapper->setView(view->main);

		auto entryEditController = factory->createController<EntryEditController>(view->entryEditor);
		auto mainController      = factory->createController<MainController>(view->main);
		auto settingsController  = factory->createController<SettingsController>(view->settings);
		auto envEditController   = factory->createController<EnvEditorController>(view->envEditor);
		auto trashController     = factory->createController<TrashController>(view->trash);
		auto installController   = factory->createController<InstallerController>(view->installer);
		auto aboutController     = factory->createController<AboutController>(view->about);
		auto errorController     = factory->createController<ErrorController>(view->error);
		auto themeController     = factory->createController<ThemeController>(view->theme);

		mainController->setSettingsBuffer(settingsOnDisk);
		mainController->setSavedListCfg(savedListCfg);

		// configure logger
		logger->setLogLevel(Logger_Stream::LOG_EVENT);
		if (argc > 1) {
			std::string logParam = argv[1];
			if (logParam == "debug") {
				logger->setLogLevel(Logger_Stream::LOG_DEBUG_ONLY);
			} else if (logParam == "log-important") {
				logger->setLogLevel(Logger_Stream::LOG_IMPORTANT);
			} else if (logParam == "quiet") {
				logger->setLogLevel(Logger_Stream::LOG_NOTHING);
			} else if (logParam == "verbose") {
				logger->setLogLevel(Logger_Stream::LOG_VERBOSE);
			}
		}

		factory->contentParserFactory->registerParser(factory->create<ContentParser_Linux>(), gettext("Linux"));
		factory->contentParserFactory->registerParser(factory->create<ContentParser_LinuxIso>(), gettext("Linux-ISO"));
		factory->contentParserFactory->registerParser(factory->create<ContentParser_Chainloader>(), gettext("Chainloader"));
		factory->contentParserFactory->registerParser(factory->create<ContentParser_Memtest>(), gettext("Memtest"));

		view->entryEditor->setAvailableEntryTypes(factory->contentParserFactory->getNames());

		factory->ruleMover->addStrategy(factory->create<Controller_Helper_RuleMover_Strategy_MoveRuleIntoSubmenu>());
		factory->ruleMover->addStrategy(factory->create<Controller_Helper_RuleMover_Strategy_MoveRuleOnSameLevelInsideProxy>());
		factory->ruleMover->addStrategy(factory->create<Controller_Helper_RuleMover_Strategy_MoveForeignRuleFromSubmenuToToplevel>());
		factory->ruleMover->addStrategy(factory->create<Controller_Helper_RuleMover_Strategy_MoveRuleOutOfSubmenu>());
		factory->ruleMover->addStrategy(factory->create<Controller_Helper_RuleMover_Strategy_MoveRuleIntoForeignSubmenu>());
		factory->ruleMover->addStrategy(factory->create<Controller_Helper_RuleMover_Strategy_MoveRuleOutOfProxyOnToplevel>());

		mainController->initAction();
		errorController->setApplicationStarted(true);

		application->applicationObject->run();
	} catch (Exception const& e) {
		logger->log(e, Logger::ERROR);
		return 1;
	}
}

