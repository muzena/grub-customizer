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

#ifndef STREAM_LOGGER_H_
#define STREAM_LOGGER_H_
#include "../Logger.hpp"
#include <ostream>
#include <string>

class Logger_Stream : public Logger {
	std::ostream* stream;
	int actionStackDepth;
public:
	enum LogLevel {
		LOG_NOTHING,
		LOG_DEBUG_ONLY,
		LOG_IMPORTANT,
		LOG_EVENT,
		LOG_VERBOSE
	} logLevel;
	Logger_Stream(std::ostream& stream) : stream(&stream), actionStackDepth(0), logLevel(LOG_NOTHING) {}

	void log(std::string const& message, Logger::Priority prio) {
		if (prio != ERROR && (
			this->logLevel == LOG_NOTHING ||
			(this->logLevel == LOG_DEBUG_ONLY && prio != Logger::DEBUG && prio != Logger::EXCEPTION) ||
			(this->logLevel == LOG_IMPORTANT && prio != Logger::IMPORTANT_EVENT) ||
			(this->logLevel == LOG_EVENT && prio != Logger::EVENT && prio != Logger::IMPORTANT_EVENT))) {
			return;
		}
		if (prio == Logger::IMPORTANT_EVENT) {
			*this->stream << " *** ";
		} else if (prio == Logger::EVENT) {
			*this->stream << "   * ";
		} else {
			*this->stream << "     ";
		}
	
		if (prio == Logger::INFO) {
			*this->stream << "[";
		}
		*this->stream << message;
		if (prio == Logger::INFO) {
			*this->stream << "]";
		}
	
		*this->stream << std::endl;
	}

	void logActionBegin(std::string const& controller, std::string const& action) {
		if (this->logLevel == LOG_DEBUG_ONLY || this->logLevel == LOG_VERBOSE) {
			actionStackDepth++;
			for (int i = 0; i < actionStackDepth; i++) {
				*this->stream << " ";
			}
			*this->stream << "-> " << controller << "/" << action << std::endl;
		}
	}

	void logActionEnd() {
		if (this->logLevel == LOG_DEBUG_ONLY || this->logLevel == LOG_VERBOSE) {
			if (actionStackDepth) {
				actionStackDepth--;
			}
			if (actionStackDepth == 0) {
				*this->stream << std::endl;
			}
		}
	}

	void logActionBeginThreaded(std::string const& controller, std::string const& action) {
		// not yet implemented
	}

	void logActionEndThreaded() {
		// not yet implemented
	}

	void setLogLevel(LogLevel level) {
		this->logLevel = level;
	}

};

#endif
