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

#ifndef GLIBREGEX_H_
#define GLIBREGEX_H_
#include <glibmm/thread.h>

#include <string>
#include <vector>
#include <map>
#include <glib.h>
#include <iostream>

#include "../Exception.hpp"
#include "../Regex.hpp"
#include "../Helper.hpp"

class Regex_GLib :
	public Regex
{
	public: std::vector<std::string> match(
		std::string const& pattern,
		std::string const& str,
		char const& escapeCharacter = '\0',
		char const& replaceCharacter = '\0'
	)
	{
		std::vector<std::string> result;
		GMatchInfo* mi = NULL;
		GRegex* gr = g_regex_new(pattern.c_str(), GRegexCompileFlags(0), GRegexMatchFlags(0), NULL);
		std::string escapedStr = escapeCharacter == '\0' ? "" : Helper::str_replace_escape(str, escapeCharacter, replaceCharacter);
		const gchar* matchStr = escapeCharacter == '\0' ? str.c_str() : escapedStr.c_str();
		bool success = g_regex_match(gr, matchStr, GRegexMatchFlags(0), &mi);
		if (!success)
			throw RegExNotMatchedException("RegEx doesn't match", __FILE__, __LINE__);

		gint match_count = g_match_info_get_match_count(mi);
		gint offset = 0;
		for (gint i = 0; i < match_count; i++){
			gint start_pos, end_pos;
			g_match_info_fetch_pos(mi, i, &start_pos, &end_pos);

			result.push_back(start_pos == -1 ? "" : str.substr(start_pos, end_pos - start_pos));
		}

		g_match_info_free(mi);
		g_regex_unref(gr);
		return result;
	}

	public: std::string replace(
		std::string const& pattern,
		std::string const& str,
		std::map<int, std::string> const& newValues,
		char const& escapeCharacter = '\0',
		char const& replaceCharacter = '\0'
	)
	{
		std::string result = str;
		GMatchInfo* mi = NULL;
		GRegex* gr = g_regex_new(pattern.c_str(), GRegexCompileFlags(0), GRegexMatchFlags(0), NULL);

		std::string escapedStr = escapeCharacter == '\0' ? "" : Helper::str_replace_escape(str, escapeCharacter, replaceCharacter);
		const gchar* matchStr = escapeCharacter == '\0' ? str.c_str() : escapedStr.c_str();

		bool success = g_regex_match(gr, matchStr, GRegexMatchFlags(0), &mi);
		if (!success)
			throw RegExNotMatchedException("RegEx doesn't match", __FILE__, __LINE__);

		gint match_count = g_match_info_get_match_count(mi);
		gint offset = 0;
		for (std::map<int, std::string>::const_iterator iter = newValues.begin(); iter != newValues.end(); iter++){
			gint start_pos, end_pos;
			g_match_info_fetch_pos(mi, iter->first, &start_pos, &end_pos);
			if (start_pos != -1 && end_pos != -1) { //ignore unmatched (optional) values
				result.replace(start_pos+offset, end_pos-start_pos, iter->second);
				offset += iter->second.length() - (end_pos-start_pos);
			}
		}

		g_match_info_free(mi);
		g_regex_unref(gr);
		return result;
	}
};

#endif /* GLIBMUTEX_H_ */
