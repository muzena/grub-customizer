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

#ifndef DEVICEDATALIST_IFACE_H_
#define DEVICEDATALIST_IFACE_H_
#include <map>
#include <string>
#include <memory>

class Model_DeviceDataListInterface : public std::map<std::string, std::map<std::string, std::string> > {
public:
	virtual inline ~Model_DeviceDataListInterface() {};

	virtual void loadData(FILE* blkidOutput)=0;
	virtual void clear()=0;
};

class Model_DeviceDataListInterface_Connection
{
	protected: std::shared_ptr<Model_DeviceDataListInterface> deviceDataList;

	public: void setDeviceDataList(std::shared_ptr<Model_DeviceDataListInterface> deviceDataList)
	{
		this->deviceDataList = deviceDataList;
	}
};

#endif
