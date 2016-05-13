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

#ifndef HELPER_GLIBTHREAD_H_INCLUDED
#define HELPER_GLIBTHREAD_H_INCLUDED
#include "Thread.hpp"

#include <functional>
#include <queue>

#include <glibmm/thread.h>
#include <glibmm/dispatcher.h>

#include <glibmm.h>

class Controller_Helper_GLibThread : public Controller_Helper_Thread
{
	private: std::queue<std::function<void ()>> dispatchQueue;
	private: Glib::Dispatcher dispatcher; // the new general dispatcher
	private: Glib::Threads::Mutex mutex;

	public: Controller_Helper_GLibThread()
	{
		this->dispatcher.connect(sigc::mem_fun(this, &Controller_Helper_GLibThread::dispatcherCallback));
	}

	public: void runDispatched(std::function<void ()> function)
	{
		Glib::Threads::Mutex::Lock lock(this->mutex);
		this->dispatchQueue.push(function);
		lock.release();

		this->dispatcher();
	}

	public: void runDelayed(std::function<void ()> function, int delayInMilliSec)
	{
		Glib::signal_timeout().connect_once(function, delayInMilliSec);
	}

	public: void runAsThread(std::function<void ()> function)
	{
		Glib::Thread::create(function, false);
	}

	private: void dispatcherCallback()
	{
		Glib::Threads::Mutex::Lock lock(this->mutex);
		auto func = this->dispatchQueue.front();
		this->dispatchQueue.pop();
		lock.release();

		func();
	}
};

#endif /* HELPER_GLIBTHREAD_H_INCLUDED */
