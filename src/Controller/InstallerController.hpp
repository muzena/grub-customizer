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

#ifndef INSTALLERCONTROLLERIMPL_H_
#define INSTALLERCONTROLLERIMPL_H_

#include <libintl.h>
#include <locale.h>
#include <sstream>
#include "../config.hpp"

#include "../Model/Env.hpp"

#include "../Model/Installer.hpp"
#include "../View/Installer.hpp"
#include "../View/Trait/ViewAware.hpp"

#include "Common/ControllerAbstract.hpp"

#include "../lib/Exception.hpp"

#include "Helper/Thread.hpp"


class InstallerController :
	public Controller_Common_ControllerAbstract,
	public View_Trait_ViewAware<View_Installer>,
	public Model_Installer_Connection,
	public Model_Env_Connection,
	public Controller_Helper_Thread_Connection,
	public Bootstrap_Application_Object_Connection
{
	public:	InstallerController() :
		Controller_Common_ControllerAbstract("installer")
	{
	}

	public:	void initViewEvents() override
	{
		using namespace std::placeholders;

		this->view->onInstallClick = std::bind(std::mem_fn(&InstallerController::installGrubAction), this, _1);
	}

	public:	void initApplicationEvents() override
	{
		using namespace std::placeholders;

		this->applicationObject->onInstallerShowRequest.addHandler(std::bind(std::mem_fn(&InstallerController::showAction), this));
	}

	public:	void initInstallerEvents() override
	{
		using namespace std::placeholders;

		this->installer->onFinish = [this] (std::string message) {
			this->threadHelper->runDispatched(std::bind(std::mem_fn(&InstallerController::showMessageAction), this, message));
		};
	}

	public:	void showAction()
	{
		this->logActionBegin("show");
		try {
			view->show();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public:	void installGrubAction(std::string device)
	{
		this->logActionBegin("install-grub");
		try {
			this->threadHelper->runAsThread(std::bind(std::mem_fn(&InstallerController::installGrubThreadedAction), this, device));
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public:	void installGrubThreadedAction(std::string device)
	{
		this->logActionBeginThreaded("install-grub-threaded");
		try {
			this->env->activeThreadCount++;
			installer->threadable_install(device);
			this->env->activeThreadCount--;
			if (this->env->activeThreadCount == 0 && this->env->quit_requested) {
				this->applicationObject->shutdown();
			}
		} catch (Exception const& e) {
			this->applicationObject->onThreadError.exec(e);
		}
		this->logActionEndThreaded();
	}

	public:	void showMessageAction(std::string const& msg)
	{
		this->logActionBegin("show-message");
		try {
			view->showMessageGrubInstallCompleted(msg);
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

};

#endif
