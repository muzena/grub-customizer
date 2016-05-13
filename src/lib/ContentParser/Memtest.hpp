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

#ifndef CONTENT_PARSER_MEMTEST_H_
#define CONTENT_PARSER_MEMTEST_H_

#include "../Regex.hpp"
#include "../../Model/DeviceMap.hpp"
#include "Abstract.hpp"

class ContentParser_Memtest :
	public ContentParser_Abstract,
	public Regex_RegexConnection,
	public Model_DeviceMap_Connection,
	public Model_MountTable_Connection,
	public Model_DeviceDataList_Connection
{
	static const char* _regex;
	std::string sourceCode;
public:
	void parse(std::string const& sourceCode) {
		this->sourceCode = sourceCode;
		try {
			std::vector<std::string> result = this->regexEngine->match(ContentParser_Memtest::_regex, this->sourceCode, '\\', '_');
	
	
			//check partition indices by uuid
			Model_DeviceMap_PartitionIndex pIndex = this->deviceMap->getHarddriveIndexByPartitionUuid(result[3]);
			if (pIndex.hddNum != result[1] || pIndex.partNum != result[2]){
				throw ParserException("parsing failed - hdd num check", __FILE__, __LINE__);
			}
	
			this->options["partition_uuid"] = result[3];
			this->options["memtest_image"] = this->unescape(result[4]);


			try {
				std::string device = this->deviceDataList->getDeviceByUuid(this->options["partition_uuid"]);
				this->options["memtest_image_full"] = Helper::rtrim(this->mountTable->findByDevice(device).mountpoint, "/") + "/" + Helper::ltrim(this->options["memtest_image"], "/");
				if (!this->_fileExists(this->options["memtest_image_full"])) {
					throw ItemNotFoundException("memtest image '" + this->options["memtest_image_full"] + "'not found!", __FILE__, __LINE__);
				}
				this->options.erase("partition_uuid");
				this->options.erase("memtest_image");
			} catch (ItemNotFoundException const& e) {
				// partition not mounted
				this->options.erase("memtest_image_full");
			}

		} catch (RegExNotMatchedException const& e) {
			throw ParserException("parsing failed - RegEx not matched", __FILE__, __LINE__);
		}
	}

	std::string buildSource() const {
		std::string partitionUuid, filePath;

		if (this->options.find("memtest_image_full") != this->options.end()) {
			std::string realMemtestPath = this->_realpath(this->options.at("memtest_image_full"));
			Model_MountTable_Mountpoint& mountpoint = this->mountTable->getByFilePath(realMemtestPath);
			partitionUuid = (*this->deviceDataList)[mountpoint.device]["UUID"];
			filePath = realMemtestPath.substr(mountpoint.mountpoint.size());
		} else {
			partitionUuid = this->options.at("partition_uuid");
			filePath = this->options.at("memtest_image");
		}

		try {
			Model_DeviceMap_PartitionIndex pIndex = this->deviceMap->getHarddriveIndexByPartitionUuid(partitionUuid);
			std::map<int, std::string> newValues;
			newValues[1] = pIndex.hddNum;
			newValues[2] = pIndex.partNum;
			newValues[3] = partitionUuid;
			newValues[4] = this->escape(filePath);

			std::string result;

			result = this->regexEngine->replace(ContentParser_Memtest::_regex, this->sourceCode, newValues, '\\', '_');
			this->regexEngine->match(ContentParser_Memtest::_regex, result, '\\', '_');

			return result;
		} catch (RegExNotMatchedException const& e) {
			throw ParserException("parsing failed - RegEx not matched", __FILE__, __LINE__);
		}
	}


	void buildDefaultEntry() {
		std::string defaultEntry =
			"set root='(hd0,0)'\n"
			"search --no-floppy --fs-uuid --set 000\n"
			"linux16 ___";

		this->sourceCode = defaultEntry;

		assert(this->regexEngine->match(ContentParser_Memtest::_regex, defaultEntry, '\\', '_').size() > 0);

		this->options.clear();
		this->options["memtest_image_full"] = "/boot/memtest86+.bin";
	}

};

const char* ContentParser_Memtest::_regex =
	"[ \t]*set root='\\(hd([0-9]+)[^0-9]+([0-9]+)\\)'\\n"
	"[ \t]*search[ \t]+--no-floppy[ \t]+--fs-uuid[ \t]+--set(?:=root)? ([-0-9a-fA-F]+)\\n"
	"[ \t]*linux16[ \t]*(\"[^\"]*\"|[^ \\t\\n]+).*$";

#endif /* CONTENT_PARSER_MEMTEST_H_ */
