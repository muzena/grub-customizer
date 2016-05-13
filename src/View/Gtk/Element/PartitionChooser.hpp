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

#ifndef PARTITIONCHOOSER_DROPDOWN_H_
#define PARTITIONCHOOSER_DROPDOWN_H_
#include <gtkmm.h>
#include <string>
#include "../../../Model/DeviceDataListInterface.hpp"
#include <libintl.h>

class View_Gtk_Element_PartitionChooser :
	public Gtk::ComboBoxText
{
	private: std::map<std::string, std::string> uuid_map;
	private: Glib::ustring activePartition_uuid;
	private: Model_DeviceDataListInterface const* deviceDataList;
	private: bool prependCurrentPartition;
	private: std::string currentPartitionName;

	public:	View_Gtk_Element_PartitionChooser(
		Glib::ustring const& activePartition_uuid,
		Model_DeviceDataListInterface const& deviceDataList,
		bool prependCurrentPartition = false,
		std::string const& currentPartitionName = ""
	) :
		activePartition_uuid(activePartition_uuid),
		deviceDataList(&deviceDataList),
		prependCurrentPartition(prependCurrentPartition),
		currentPartitionName(currentPartitionName)
	{
		load();
	}

	public:	void load()
	{
		this->remove_all();
		if (prependCurrentPartition) {
			this->append(currentPartitionName + "\n(" + gettext("current") + ")");
			this->set_active(0);
		}
		for (Model_DeviceDataListInterface::const_iterator iter = deviceDataList->begin(); iter != deviceDataList->end(); iter++) {
			if (iter->second.find("UUID") != iter->second.end()) {
				Glib::ustring text = iter->first + "\n(" + (iter->second.find("LABEL") != iter->second.end() ? iter->second.at("LABEL") + ", " : "") + (iter->second.find("TYPE") != iter->second.end() ? iter->second.at("TYPE") : "") + ")";
				uuid_map[text] = iter->second.at("UUID");
				this->append(text);
				if (strToLower(iter->second.at("UUID")) == strToLower(activePartition_uuid)) {
					this->set_active_text(text);
				}
			}
		}
	}

	public:	std::string getSelectedUuid() const
	{
		if (this->get_active_row_number() == 0 && prependCurrentPartition) { // (current)
			return "";
		} else {
			return this->uuid_map.at(this->get_active_text());
		}
	}

	public:	static std::string strToLower(std::string str)
	{
		for (std::string::iterator iter = str.begin(); iter != str.end(); iter++) {
			*iter = std::tolower(*iter);
		}
		return str;
	}

};

#endif
