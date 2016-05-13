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

#ifndef REGEX_H_INCLUDED
#define REGEX_H_INCLUDED
#include <string>
#include <vector>
#include <map>
#include <memory>

class Regex {
	public: virtual ~Regex(){}
	public: virtual std::vector<std::string> match(
		std::string const& pattern,
		std::string const& str,
		char const& escapeCharacter = '\0',
		char const& replaceCharacter = '\0'
	) = 0;

	public: virtual std::string replace(
		std::string const& pattern,
		std::string const& str,
		std::map<int, std::string> const& newValues,
		char const& escapeCharacter = '\0',
		char const& replaceCharacter = '\0'
	) = 0;
};

class Regex_RegexConnection
{
	protected: std::shared_ptr<Regex> regexEngine;

	public: void setRegexEngine(std::shared_ptr<Regex> regexEngine)
	{
		this->regexEngine = regexEngine;
	}
};

#endif
