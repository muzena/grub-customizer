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

#ifndef ERRORCONTROLLERIMPL_H_
#define ERRORCONTROLLERIMPL_H_

#include <libintl.h>
#include <locale.h>
#include <sstream>
#include "../config.hpp"

#include "../Model/Env.hpp"

#include "../View/Error.hpp"
#include "../View/Trait/ViewAware.hpp"

#include "Common/ControllerAbstract.hpp"
#include "Helper/Thread.hpp"


class ErrorController :
	public Controller_Common_ControllerAbstract,
	public View_Trait_ViewAware<View_Error>,
	public Controller_Helper_Thread_Connection,
	public Bootstrap_Application_Object_Connection
{
	private: bool applicationStarted;

	public:	void setApplicationStarted(bool val)
	{
		this->applicationStarted = val;
	}

	public:	ErrorController() :
		Controller_Common_ControllerAbstract("error"),
		applicationStarted(false)
	{
	}

	public:	void initViewEvents() override
	{
		using namespace std::placeholders;

		this->view->onQuitClick = std::bind(std::mem_fn(&ErrorController::quitAction), this);
	}

	public:	void initApplicationEvents() override
	{
		using namespace std::placeholders;

		this->applicationObject->onError.addHandler(std::bind(std::mem_fn(&ErrorController::errorAction), this, _1));
		this->applicationObject->onThreadError.addHandler(std::bind(std::mem_fn(&ErrorController::errorThreadedAction), this, _1));
	}

	
	public:	void errorAction(Exception const& e)
	{
		this->log(e, Logger::EXCEPTION);
		this->view->showErrorMessage(e, this->applicationStarted);
	}

	public:	void errorThreadedAction(Exception const& e)
	{
		if (this->threadHelper) {
			this->threadHelper->runDispatched(std::bind(std::mem_fn(&ErrorController::errorAction), this, Exception(e)));
		} else {
			this->log(e, Logger::EXCEPTION);
			exit(1);
		}
	}

	public:	void quitAction()
	{
		if (this->applicationStarted) {
			this->applicationObject->shutdown();
		} else {
			exit(2);
		}
	}

};

#endif
