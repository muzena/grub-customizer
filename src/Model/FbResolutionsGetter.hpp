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

#ifndef FB_RESOLUTIONS_GETTER
#define FB_RESOLUTIONS_GETTER
#include <string>
#include <list>
#include <cstdio>
#include <functional>
#include "../lib/Trait/LoggerAware.hpp"

class Model_FbResolutionsGetter : public Trait_LoggerAware {
	std::list<std::string> data;
	bool _isLoading;
public:
	Model_FbResolutionsGetter() : _isLoading(false)
	{}

	std::function<void ()> onFinish;

	const std::list<std::string>& getData() const {
		return data;
	}

	void load() {
		if (!_isLoading){ //make sure that only one thread is running this function at the same time
			_isLoading = true;
			data.clear();
			FILE* hwinfo_proc = popen("hwinfo --framebuffer", "r");
			if (hwinfo_proc){
				int c;
				std::string row;
				//parses mode lines like "  Mode 0x0300: 640x400 (+640), 8 bits"
				while ((c = fgetc(hwinfo_proc)) != EOF){
					if (c != '\n')
						row += char(c);
					else {
						if (row.substr(0,7) == "  Mode "){
							int beginOfResulution = row.find(':')+2;
							int endOfResulution = row.find(' ', beginOfResulution);
	
							int beginOfColorDepth = row.find(' ', endOfResulution+1)+1;
							int endOfColorDepth = row.find(' ', beginOfColorDepth);
	
							data.push_back(
								row.substr(beginOfResulution, endOfResulution-beginOfResulution)
							  + "x"
							  + row.substr(beginOfColorDepth, endOfColorDepth-beginOfColorDepth)
							);
						}
						row = "";
					}
				}
				if (pclose(hwinfo_proc) == 0 && this->onFinish) {
					this->onFinish();
				}
			}
			_isLoading = false;
		}
	}

};

class Model_FbResolutionsGetter_Connection
{
	protected: std::shared_ptr<Model_FbResolutionsGetter> fbResolutionsGetter;

	public: virtual ~Model_FbResolutionsGetter_Connection() {}

	public: void setFbResolutionsGetter(std::shared_ptr<Model_FbResolutionsGetter> fbResolutionsGetter)
	{
		this->fbResolutionsGetter = fbResolutionsGetter;

		this->initFbResolutionsGetterEvents();
	}

	public: virtual void initFbResolutionsGetterEvents()
	{
		// override to initialize specific view events
	}
};

#endif
