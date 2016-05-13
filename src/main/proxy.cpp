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

#include "../Model/Entry.hpp"
#include <iostream>
#include <memory>
#include "../Model/ListCfg.hpp" // multi
#include "../Model/Proxy.hpp"
#include "../Model/Rule.hpp"
#include "../Model/Script.hpp"

int main(int argc, char** argv){
	if (argc == 2) {
		auto script = std::make_shared<Model_Script>("noname", "");
		std::shared_ptr<Model_Entry> newEntry;
		std::string plaintextBuffer;
		while (*(newEntry = std::make_shared<Model_Entry>(stdin, Model_Entry_Row(), nullptr, &plaintextBuffer))) {
			script->entries().push_back(newEntry);
		}
		if (plaintextBuffer.size()) {
			script->entries().push_front(std::make_shared<Model_Entry>("#text", "", plaintextBuffer, Model_Entry::PLAINTEXT));
		}

		auto proxy = std::make_shared<Model_Proxy>();
		proxy->importRuleString(argv[1], "");

		proxy->dataSource = script;
		proxy->sync(true, true);
		
		for (auto rule : proxy->rules) {
			rule->print(std::cout);
		}
		return 0;
	} else if (argc == 3 && std::string(argv[2]) == "multi") {
		auto env = std::make_shared<Model_Env>();
		Model_ListCfg scriptSource;
		scriptSource.setEnv(env);
		scriptSource.ignoreLock = true;
		{ // this scope prevents access to the unused proxy variable - push_back takes a copy!
			auto proxy = std::make_shared<Model_Proxy>();
			proxy->importRuleString(argv[1], env->cfg_dir_prefix);
			scriptSource.proxies.push_back(proxy);
		}
		scriptSource.readGeneratedFile(stdin, true, false);

		scriptSource.proxies.front()->dataSource = scriptSource.repository.front(); // the first Script is always the main script

		auto map = scriptSource.repository.getScriptPathMap();
		scriptSource.proxies.front()->sync(true, true, map);

		for (auto& rule : scriptSource.proxies.front()->rules) {
			rule->print(std::cout);
		}
	} else {
		std::cerr << "wrong argument count. You have to give the config as parameter 1!" << std::endl;
		return 1;
	}
}
