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

#ifndef CONTENT_PARSER_CHAINLOADER_H_
#define CONTENT_PARSER_CHAINLOADER_H_

#include "../Regex.hpp"
#include "../../Model/DeviceMap.hpp"
#include "Abstract.hpp"

class ContentParser_Chainloader :
	public ContentParser_Abstract,
	public Regex_RegexConnection,
	public Model_DeviceMap_Connection
{
	static const char* _regex;
	std::string sourceCode;
public:
	void parse(std::string const& sourceCode) {
		this->sourceCode = sourceCode;
		try {
			std::vector<std::string> result = this->regexEngine->match(ContentParser_Chainloader::_regex, this->sourceCode, '\\', '_');
	
			//check partition indices by uuid
			Model_DeviceMap_PartitionIndex pIndex = this->deviceMap->getHarddriveIndexByPartitionUuid(result[3]);
			if (pIndex.hddNum != result[1] || pIndex.partNum != result[2]){
				throw ParserException("parsing failed - hdd num check", __FILE__, __LINE__);
			}
	
			this->options["partition_uuid"] = result[3];
		} catch (RegExNotMatchedException const& e) {
			throw ParserException("parsing failed - RegEx not matched", __FILE__, __LINE__);
		}
	}

	std::string buildSource() const {
		try {
			Model_DeviceMap_PartitionIndex pIndex = deviceMap->getHarddriveIndexByPartitionUuid(this->options.at("partition_uuid"));
			std::map<int, std::string> newValues;
			newValues[1] = pIndex.hddNum;
			newValues[2] = pIndex.partNum;
			newValues[3] = this->options.at("partition_uuid");

			std::string result;

			result = this->regexEngine->replace(ContentParser_Chainloader::_regex, this->sourceCode, newValues, '\\', '_');
			this->regexEngine->match(ContentParser_Chainloader::_regex, result, '\\', '_');

			return result;
		} catch (RegExNotMatchedException const& e) {
			throw ParserException("parsing failed - RegEx not matched", __FILE__, __LINE__);
		}
	}

	void buildDefaultEntry() {
		std::string defaultEntry =
			"set root='(hd0,0)'\n"
			"search --no-floppy --fs-uuid --set 000\n"
			"drivemap -s (hd0) ${root}\n"
			"chainloader +1";

		assert(this->regexEngine->match(ContentParser_Chainloader::_regex, defaultEntry, '\\', '_').size() > 0);

		this->sourceCode = defaultEntry;

		this->options.clear();
		this->options["partition_uuid"] = "";
	}
};

const char* ContentParser_Chainloader::_regex =
	"[ \t]*set[ \t]+root='\\(hd([0-9]+)[^0-9]+([0-9]+)\\)'\\n"
	"[ \t]*search[ \t]+--no-floppy[ \t]+--fs-uuid[ \t]+--set(?:=root)?[ \t]+([-0-9a-fA-F]+)\\n"
	"(.|\\n)*"
	"[ \t]*chainloader[ \t]+\\+1\\n?[ \t]*";
#endif /* CONTENT_PARSER_CHAINLOADER_H_ */
