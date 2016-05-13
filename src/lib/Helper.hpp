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
#ifndef HELPER_H_INCLUDED
#define HELPER_H_INCLUDED

#include <cstdio>
#include <openssl/md5.h>
#include <string>
#include <map>
#include "Exception.hpp"

# define ASSERT_VOID_CAST static_cast<void>

# define assert(expr)							\
  ((expr)								\
   ? ASSERT_VOID_CAST (0)						\
   : Helper::assert_fail (__STRING(expr), __FILE__, __LINE__, __func__))

class Helper {
	public: static void assert_fail(std::string const& expr, std::string const& file, int line, std::string const& func) {
		throw AssertException("Assertion `" + expr + "' failed. Function: " + func, file, line);
	}

	public: static void assert_filepath_empty(std::string const& filepath, std::string const& sourceCodeFile, int line) {
		FILE* file = fopen(filepath.c_str(), "r");
		if (file) {
			fclose(file);
			throw AssertException("found unexpected file on path: " + filepath, sourceCodeFile, line);
		}
	}

	public: static std::string md5(std::string const& input) {
		unsigned char buf[16];
		unsigned char* cStr = new unsigned char[input.length() + 1];
		for (int i = 0; i < input.length(); i++) {
			cStr[i] = static_cast<unsigned char>(input[i]);
		}
		MD5(cStr, input.length(), buf);

		std::string result;
		for (int i = 0; i < 16; i++) {
			unsigned int a = (buf[i] - (buf[i] % 16)) / 16;
			if (a <= 9) {
				result += '0' + a;
			} else {
				result += 'a' + a - 10;
			}
			unsigned int b = (buf[i] % 16);
			if (b <= 9) {
				result += '0' + b;
			} else {
				result += 'a' + b - 10;
			}
		}
		assert(result.length() == 32);
		return result;
	}

	public: static std::string str_replace(const std::string &search, const std::string &replace, std::string subject) {
		size_t pos = 0;
		while (pos < subject.length() && (pos = subject.find(search, pos)) != -1){
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return subject;
	}

	public: static std::string escapeXml(std::string const& input) {
		return str_replace("<", "&lt;", str_replace("&", "&amp;", input));
	}

	public: static std::string ltrim(std::string string, std::string const& chars = " \t\n\r") {
		int first = string.find_first_not_of(chars);
		if (first != -1) {
			return string.substr(first);
		} else {
			return "";
		}
	}

	public: static std::string rtrim(std::string string, std::string const& chars = " \t\n\r") {
		string = std::string(string.rbegin(), string.rend());
		string = ltrim(string, chars);
		string = std::string(string.rbegin(), string.rend());
		return string;
	}

	public: static std::string trim(std::string string, std::string const& chars = " \t\n\r") {
		return rtrim(ltrim(string, chars));
	}

	public: static std::string str_replace_escape(std::string subject, char const& escapeCharacter, char const& replaceCharacter) {
		std::map<int, std::string> extractedEscapeSequences;

		size_t pos = 0;
		while (pos < subject.length() && (pos = subject.find(escapeCharacter, pos)) != -1){
			subject[pos] = replaceCharacter;
			subject[pos + 1] = replaceCharacter;
			pos += 2;
		}

		return subject;
	}

	public: static std::string str_escape(std::string subject, char const& escapeCharacter, std::string const& charactersToBeEscaped) {
		size_t pos = 0;
		while (pos < subject.length() && (pos = subject.find_first_of(charactersToBeEscaped, pos)) != -1){
			subject.replace(pos, 1, std::string(1, escapeCharacter) + subject[pos]);
			pos += 2;
		}
		return subject;
	}

	public: static std::string str_unescape(std::string subject, char const& escapeCharacter) {
		size_t pos = 0;
		while (pos < subject.length() && (pos = subject.find_first_of(escapeCharacter, pos)) != -1){
			subject.replace(pos, 2, subject.substr(pos + 1, 1));
			pos += 1;
		}
		return subject;
	}
};


#endif
