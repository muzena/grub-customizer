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


#ifndef ENTRYPATHBUILDERIMPL_H_
#define ENTRYPATHBUILDERIMPL_H_
#include "../Model/EntryPathBuilder.hpp"
#include <map>
#include "Script.hpp"

class Model_EntryPathBuilderImpl : public Model_EntryPathBilder
{
	private: std::shared_ptr<Model_Script> mainScript;
	private: std::map<std::shared_ptr<Model_Entry>, std::shared_ptr<Model_Script>> entrySourceMap;
	private: std::map<std::shared_ptr<Model_Script>, std::string> scriptTargetMap;
	private: int prefixLength;

	public: Model_EntryPathBuilderImpl(std::shared_ptr<Model_Script> mainScript) : prefixLength(0), mainScript(NULL)
	{
		this->setMainScript(mainScript);
	}

	public: void setMainScript(std::shared_ptr<Model_Script> mainScript)
	{
		this->mainScript = mainScript;
	}

	public: void setEntrySourceMap(std::map<std::shared_ptr<Model_Entry>, std::shared_ptr<Model_Script>> const& entrySourceMap)
	{
		this->entrySourceMap = entrySourceMap;
	}

	public: void setScriptTargetMap(std::map<std::shared_ptr<Model_Script>, std::string> const& scriptTargetMap)
	{
		this->scriptTargetMap = scriptTargetMap;
	}

	public: void setPrefixLength(int length)
	{
		this->prefixLength = length;
	}

	public: std::list<std::string> buildPath(std::shared_ptr<Model_Entry> entry) const
	{
		auto script = entrySourceMap.find(entry) != entrySourceMap.end() ? entrySourceMap.find(entry)->second : this->mainScript;
		return script->buildPath(entry);
	}

	public: std::string buildPathString(std::shared_ptr<Model_Entry> entry, bool withOtherEntriesPlaceholder = false) const
	{
		auto script = entrySourceMap.find(entry) != entrySourceMap.end() ? entrySourceMap.find(entry)->second : this->mainScript;
		return script->buildPathString(entry, withOtherEntriesPlaceholder);
	}

	public: std::string buildScriptPath(std::shared_ptr<Model_Entry> entry) const
	{
		auto script = entrySourceMap.find(entry) != entrySourceMap.end() ? entrySourceMap.find(entry)->second : nullptr;
		return script ? this->scriptTargetMap.find(script)->second.substr(this->prefixLength) : "";
	}

};

#endif
