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

#ifndef CONTENT_PARSER_ABSTRACT_H_
#define CONTENT_PARSER_ABSTRACT_H_
#include <map>
#include <string>
#include <climits>
#include <cstdlib>
#include <cstdio>
#include "../../lib/ContentParser.hpp"
#include "../../lib/Trait/LoggerAware.hpp"
#include "../../lib/Helper.hpp"

class ContentParser_Abstract :
	public ContentParser,
	public Trait_LoggerAware
{
	protected: std::map<std::string, std::string> options;

	public:	virtual inline ~ContentParser_Abstract() {}

	public:	std::map<std::string, std::string> getOptions() const
	{
		return this->options;
	}

	public:	bool hasOption(std::string const& name) const
	{
		return this->options.find(name) != this->options.end();
	}

	public:	std::string getOption(std::string const& name) const
	{
		if (!this->hasOption(name)) {
			throw ItemNotFoundException("option '" + name + "' not found", __FILE__, __LINE__);
		}

		return this->options.at(name);
	}

	public:	void setOption(std::string const& name, std::string const& value)
	{
		this->options[name] = value;
	}

	public:	void setOptions(std::map<std::string, std::string> const& options)
	{
		this->options = options;
	}

	public:	std::list<std::string> getErrors()
	{
		std::list<std::string> errors;

		// not empty
		for (std::map<std::string, std::string>::iterator optionIter = this->options.begin(); optionIter != this->options.end(); optionIter++) {
			if (optionIter->first == "other_params") {
				continue;
			}

			if (optionIter->second == "") {
				errors.push_back(optionIter->first);
			}
		}

		// specific validators
		if (this->options.find("iso_path_full") != this->options.end() && !this->_fileExists(this->options["iso_path_full"])) {
			errors.push_back("iso_path_full");
		}
		if (this->options.find("memtest_image_full") != this->options.end() && !this->_fileExists(this->options["memtest_image_full"])) {
			errors.push_back("memtest_image_full");
		}

		errors.unique();
		return errors;
	}

	protected: std::string _realpath(std::string const& path) const
	{
		char* realPathCharPtr = realpath(path.c_str(), NULL);
		std::string realPath;
		if (realPathCharPtr != NULL) {
			realPath = realPathCharPtr;
			free(realPathCharPtr); // allocated data by realpath must be deleted
		} else {
			realPath = path; // use given path if file not found
		}
		return realPath;
	}

	protected: bool _fileExists(std::string const& fileName)
	{
		FILE* file = fopen(fileName.c_str(), "r");
		if (file) {
			fclose(file);
			return true;
		} else {
			return false;
		}
	}

	protected: std::string escape(std::string value) const
	{
		if (value.find_first_of(" \"") != -1 || value.size() == 0) {
			value = "\"" + Helper::str_escape(value, '\\', "\\\"") + "\"";
		}
		return value;
	}

	protected: std::string unescape(std::string value) const
	{
		if (value[0] == '"' && value[value.size() - 1] == '"') {
			value = value.substr(1, value.size() - 2);
		}
		return Helper::str_unescape(value, '\\');
	}
};

#endif /* CONTENT_PARSER_ABSTRACT_H_ */
