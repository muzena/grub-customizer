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

#ifndef HELPER_THREAD_H_INCLUDED
#define HELPER_THREAD_H_INCLUDED
#include "../../lib/Trait/LoggerAware.hpp"
#include <functional>

class Controller_Helper_Thread : public Trait_LoggerAware
{
	public: virtual inline ~Controller_Helper_Thread() {};
	public: virtual void runDispatched(std::function<void ()> function) = 0;
	public: virtual void runDelayed(std::function<void ()> function, int delayInMilliSec) = 0;
	public: virtual void runAsThread(std::function<void ()> function) = 0;
};

class Controller_Helper_Thread_Connection
{
	protected: std::shared_ptr<Controller_Helper_Thread> threadHelper;

	public: virtual ~Controller_Helper_Thread_Connection(){}

	public: void setThreadHelper(std::shared_ptr<Controller_Helper_Thread> threadHelper)
	{
		this->threadHelper = threadHelper;
	}
};

#endif /* HELPER_THREAD_H_INCLUDED */
