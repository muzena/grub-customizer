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

#ifndef THEMEFILE_H_
#define THEMEFILE_H_
#include <string>

struct Model_ThemeFile {
	Model_ThemeFile(std::string localFileName, bool isAddedByUser = false) : localFileName(localFileName), contentLoaded(false), newLocalFileName(localFileName), isAddedByUser(isAddedByUser)
	{
	}

	static bool compareLocalPath(Model_ThemeFile const& a, Model_ThemeFile const& b) {
		return a.newLocalFileName < b.newLocalFileName;
	}

	std::string localFileName, newLocalFileName; // path inside of the theme directory
	bool contentLoaded; // say whether the content is loaded (text only)
	std::string content; // loaded content (text only)
	bool isAddedByUser;
	std::string externalSource;
};


#endif /* THEMEFILE_H_ */
