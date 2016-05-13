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

#ifndef GRUB_CUSTOMIZER_GrublistCfg_INCLUDED
#define GRUB_CUSTOMIZER_GrublistCfg_INCLUDED
#include <list>
#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <map>
#include <libintl.h>
#include <unistd.h>
#include <fstream>

#include "../config.hpp"

#include "../lib/Mutex.hpp"
#include "../lib/Trait/LoggerAware.hpp"

#include "../lib/Exception.hpp"
#include "../lib/ArrayStructure.hpp"
#include "../lib/Helper.hpp"
#include <stack>
#include <algorithm>
#include <functional>
#include "Env.hpp"
#include "MountTable.hpp"
#include "Proxylist.hpp"
#include "ProxyScriptData.hpp"
#include "Repository.hpp"
#include "ScriptSourceMap.hpp"
#include "SettingsManagerData.hpp"

class Model_ListCfg :
	public Trait_LoggerAware,
	public Mutex_Connection,
	public Model_Env_Connection
{
	private: double progress;
	private: std::string progress_name;
	private: int progress_pos, progress_max;
	private: std::string errorLogFile;

	private: Model_ScriptSourceMap scriptSourceMap;

	public: Model_ListCfg() : error_proxy_not_found(false),
	 progress(0),
	 cancelThreadsRequested(false), verbose(true),
	 errorLogFile(ERROR_LOG_FILE), ignoreLock(false), progress_pos(0), progress_max(0)
	{}

	public: void initLogger() override {
		this->proxies.setLogger(this->logger);
		this->repository.setLogger(this->logger);
		this->scriptSourceMap.setLogger(this->logger);
	}

	public: void initEnv() override {
		this->scriptSourceMap.setEnv(this->env);
	}


	public: Model_Proxylist proxies;
	public: Model_Repository repository;
	
	public: std::function<void ()> onLoadStateChange;
	public: std::function<void ()> onSaveStateChange;

	public: bool verbose;
	public: bool error_proxy_not_found;
	public: void lock() {
		if (this->ignoreLock)
			return;
		if (this->mutex == NULL)
			throw ConfigException("missing mutex", __FILE__, __LINE__);
		this->mutex->lock();
	}

	public: bool lock_if_free() {
		if (this->ignoreLock)
			return true;
		if (this->mutex == NULL)
			throw ConfigException("missing mutex", __FILE__, __LINE__);
		return this->mutex->trylock();
	}

	public: void unlock() {
		if (this->ignoreLock)
			return;
		if (this->mutex == NULL)
			throw ConfigException("missing mutex", __FILE__, __LINE__);
		this->mutex->unlock();
	}

	public: bool ignoreLock;
	
	public: bool cancelThreadsRequested;

	public: bool createScriptForwarder(std::string const& scriptName) const
	{
		//replace: $cfg_dir/proxifiedScripts/ -> $cfg_dir/LS_
		std::string scriptNameNoPath = scriptName.substr((this->env->cfg_dir+"/proxifiedScripts/").length());
		std::string outputFilePath = this->env->cfg_dir+"/LS_"+scriptNameNoPath;
		FILE* existingScript = fopen(outputFilePath.c_str(), "r");
		if (existingScript == NULL){
			Helper::assert_filepath_empty(outputFilePath, __FILE__, __LINE__);
			FILE* fwdScript = fopen(outputFilePath.c_str(), "w");
			if (fwdScript){
				fputs("#!/bin/sh\n", fwdScript);
				fputs(("'"+scriptName.substr(env->cfg_dir_prefix.length())+"'").c_str(), fwdScript);
				fclose(fwdScript);
				chmod(outputFilePath.c_str(), 0755);
				return true;
			}
			else
				return false;
		}
		else {
			fclose(existingScript);
			return false;
		}
	}

	public: bool removeScriptForwarder(std::string const& scriptName) const
	{
		std::string scriptNameNoPath = scriptName.substr((this->env->cfg_dir+"/proxifiedScripts/").length());
		std::string filePath = this->env->cfg_dir+"/LS_"+scriptNameNoPath;
		return unlink(filePath.c_str()) == 0;
	}

	public: std::string readScriptForwarder(std::string const& scriptForwarderFilePath) const {
		std::string result;
		FILE* scriptForwarderFile = fopen(scriptForwarderFilePath.c_str(), "r");
		if (scriptForwarderFile){
			int c;
			while ((c = fgetc(scriptForwarderFile)) != EOF && c != '\n'){} //skip first line
			if (c != EOF)
				while ((c = fgetc(scriptForwarderFile)) != EOF && c != '\n'){result += char(c);} //read second line (=path)
			fclose(scriptForwarderFile);
		}
		if (result.length() >= 3) {
			return result.substr(1, result.length()-2);
		} else {
			return "";
		}
	}

	public: void load(bool preserveConfig = false)
	{
		if (!preserveConfig){
			send_new_load_progress(0);
	
			DIR* hGrubCfgDir = opendir(this->env->cfg_dir.c_str());
	
			if (!hGrubCfgDir){
				throw DirectoryNotFoundException("grub cfg dir not found", __FILE__, __LINE__);
			}
	
			//load scripts
			this->log("loading scripts…", Logger::EVENT);
			this->lock();
			repository.load(this->env->cfg_dir, false);
			repository.load(this->env->cfg_dir+"/proxifiedScripts", true);
			this->unlock();
			send_new_load_progress(0.05);
		
		
			//load proxies
			this->log("loading proxies…", Logger::EVENT);
			this->lock();
			struct dirent *entry;
			struct stat fileProperties;
			while ((entry = readdir(hGrubCfgDir))){
				stat((this->env->cfg_dir+"/"+entry->d_name).c_str(), &fileProperties);
				if ((fileProperties.st_mode & S_IFMT) != S_IFDIR){ //ignore directories
					if (entry->d_name[2] == '_'){ //check whether it's an script (they should be named XX_scriptname)…
						this->proxies.push_back(std::make_shared<Model_Proxy>());
						this->proxies.back()->fileName = this->env->cfg_dir+"/"+entry->d_name;
						this->proxies.back()->index = (entry->d_name[0]-'0')*10 + (entry->d_name[1]-'0');
						this->proxies.back()->permissions = fileProperties.st_mode & ~S_IFMT;
					
						FILE* proxyFile = fopen((this->env->cfg_dir+"/"+entry->d_name).c_str(), "r");
						Model_ProxyScriptData data(proxyFile);
						fclose(proxyFile);
						if (data){
							this->proxies.back()->dataSource = repository.getScriptByFilename(this->env->cfg_dir_prefix+data.scriptCmd);
							this->proxies.back()->importRuleString(data.ruleString.c_str(), this->env->cfg_dir_prefix);
						}
						else {
							this->proxies.back()->dataSource = repository.getScriptByFilename(this->env->cfg_dir+"/"+entry->d_name);
							this->proxies.back()->importRuleString("+*", this->env->cfg_dir_prefix); //it's no proxy, so accept all
						}
					
					}
				}
			}
			closedir(hGrubCfgDir);
			this->proxies.sort();
			this->unlock();
	
	
			//clean up proxy configuration
			this->log("cleaning up proxy configuration…", Logger::EVENT);
			this->lock();
	
			bool proxyRemoved = false;
			do {
				for (auto proxy : this->proxies) {
					if (!proxy->isExecutable() || !proxy->hasVisibleRules()) {
						this->log(proxy->fileName + " has no visible entries and will be removed / disabled", Logger::INFO);
						this->proxies.deleteProxy(proxy);
						proxyRemoved = true;
						break;
					}
				}
				proxyRemoved = false;
			} while (proxyRemoved);
	
			this->unlock();
		} else {
			this->lock();
			proxies.unsync_all();
			repository.deleteAllEntries();
			this->unlock();
		}
	
		//create proxifiedScript links & chmod other files
		this->log("creating proxifiedScript links & chmodding other files…", Logger::EVENT);
	
		this->lock();
		for (auto script : this->repository) {
			if (script->isInScriptDir(env->cfg_dir)){
				//createScriptForwarder & disable proxies
				createScriptForwarder(script->fileName);
				auto relatedProxies = proxies.getProxiesByScript(script);
				for (auto proxy : relatedProxies) {
					int res = chmod(proxy->fileName.c_str(), 0644);
				}
			} else {
				//enable scripts (unproxified), in this case, Proxy::fileName == Script::fileName
				chmod(script->fileName.c_str(), 0755);
			}
		}
		this->unlock();
		send_new_load_progress(0.1);
	
	
		if (!preserveConfig){
			//load script map
			this->scriptSourceMap.load();
			if (!this->scriptSourceMap.fileExists() && this->getProxifiedScripts().size() > 0) {
				this->generateScriptSourceMap();
			}
			this->populateScriptSourceMap();
		}
	
		//run mkconfig
		this->log("running " + this->env->mkconfig_cmd, Logger::EVENT);
		FILE* mkconfigProc = popen((this->env->mkconfig_cmd + " 2> " + this->errorLogFile).c_str(), "r");
		readGeneratedFile(mkconfigProc);
		
		int success = pclose(mkconfigProc);
		if (success != 0 && !cancelThreadsRequested){
			throw CmdExecException("failed running " + this->env->mkconfig_cmd, __FILE__, __LINE__);
		} else {
			remove(errorLogFile.c_str()); //remove file, if everything was ok
		}
		this->log("mkconfig successfull completed", Logger::INFO);
	
		this->send_new_load_progress(0.9);
	
		mkconfigProc = NULL;
		
		this->env->useDirectBackgroundProps = this->repository.getScriptByName("debian_theme") == NULL;
		if (this->env->useDirectBackgroundProps) {
			this->log("using simple background image settings", Logger::INFO);
		} else {
			this->log("using /usr/share/desktop-base/grub_background.sh to configure colors and the background image", Logger::INFO);
		}
	
		
		//restore old configuration
		this->log("restoring grub configuration", Logger::EVENT);
		this->lock();
		for (auto script : this->repository){
			if (script->isInScriptDir(env->cfg_dir)){
				//removeScriptForwarder & reset proxy permissions
				bool result = removeScriptForwarder(script->fileName);
				if (!result) {
					this->log("removing of script forwarder not successful!", Logger::ERROR);
				}
			}
			auto relatedProxies = proxies.getProxiesByScript(script);
			for (auto proxy : relatedProxies){
				chmod(proxy->fileName.c_str(), proxy->permissions);
			}
		}
		this->unlock();
		
		//remove invalid proxies from list (no file system action here)
		this->log("removing invalid proxies from list", Logger::EVENT);
		std::string invalidProxies = "";
		bool foundInvalidScript = false;
		do {
			foundInvalidScript = false;
			for (auto proxy : this->proxies) {
				if (proxy->dataSource == nullptr) {
					this->proxies.trash.push_back(proxy); // mark for deletion
					this->proxies.erase(this->proxies.getIter(proxy));
					foundInvalidScript = true;
					invalidProxies += proxy->fileName + ",";
					break;
				}
			}
		} while (foundInvalidScript);
	
		if (invalidProxies != "") {
			this->log("found invalid proxies: " + Helper::rtrim(invalidProxies, ","), Logger::INFO);
		}
	
		//fix conflicts (same number, same name but one script with "-proxy" the other without
		if (this->proxies.hasConflicts()) {
			this->log("found conflicts - renumerating", Logger::INFO);
			this->renumerate();
		}
	
		this->log("loading completed", Logger::EVENT);
		send_new_load_progress(1);
	}

	public: void save()
	{
		send_new_save_progress(0);
		std::map<std::string, int> samename_counter;
		proxies.deleteAllProxyscriptFiles();  //delete all proxies to get a clean file system
		proxies.clearTrash(); //delete all files of removed proxies
		repository.clearTrash();
		
		std::map<std::shared_ptr<Model_Script>, std::string> scriptFilenameMap; // stores original filenames
		for (auto script : this->repository) {
			scriptFilenameMap[script] = script->fileName;
		}
	
		// create virtual custom scripts on file system
		for (auto script : this->repository) {
			if (script->isCustomScript && script->fileName == "") {
				script->fileName = this->env->cfg_dir + "/IN_" + script->name;
				this->repository.createScript(script, "");
			}
		}
	
		for (auto script : repository) {
			script->moveToBasedir(this->env->cfg_dir);
		}
	
		send_new_save_progress(0.1);
	
		int mkdir_result = mkdir((this->env->cfg_dir+"/proxifiedScripts").c_str(), 0755); //create this directory if it doesn't already exist
	
		// get new script locations
		std::map<std::shared_ptr<Model_Script>, std::string> scriptTargetMap; // scripts and their target directories
		for (auto script : repository) {
			auto relatedProxies = proxies.getProxiesByScript(script);
			if (proxies.proxyRequired(script)){
				scriptTargetMap[script] = this->env->cfg_dir+"/proxifiedScripts/"+Model_PscriptnameTranslator::encode(script->name, samename_counter[script->name]++);
			} else {
				std::ostringstream nameStream;
				nameStream << std::setw(2) << std::setfill('0') << relatedProxies.front()->index << "_" << script->name;
				scriptTargetMap[script] = this->env->cfg_dir+"/"+nameStream.str();
			}
		}
	
		// move scripts and create proxies
		int proxyCount = 0;
		for (auto script : repository) {
			auto relatedProxies = proxies.getProxiesByScript(script);
			if (proxies.proxyRequired(script)){
				script->moveFile(scriptTargetMap[script], 0755);
				for (auto proxy : relatedProxies) {
					auto entrySourceMap = this->getEntrySources(proxy);
					std::ostringstream nameStream;
					nameStream << std::setw(2) << std::setfill('0') << proxy->index << "_" << script->name << "_proxy";
					proxy->generateFile(
						this->env->cfg_dir + "/" + nameStream.str(),
						this->env->cfg_dir_prefix.length(),
						this->env->cfg_dir_noprefix,
						entrySourceMap,
						scriptTargetMap
					);
					proxyCount++;
				}
			}
			else {
				if (relatedProxies.size() == 1){
					script->moveFile(scriptTargetMap[script], relatedProxies.front()->permissions);
					relatedProxies.front()->fileName = script->fileName; // update filename
				}
				else {
					this->log("GrublistCfg::save: cannot move proxy… only one expected!", Logger::ERROR);
				}
			}	
		}
		send_new_save_progress(0.2);
	
		// register in script source map
		for (auto scriptFilenameMapItem : scriptFilenameMap) {
			this->scriptSourceMap.registerMove(scriptFilenameMapItem.second, scriptFilenameMapItem.first->fileName);
		}
		this->scriptSourceMap.save();
	
		//remove "proxifiedScripts" dir, if empty
		
		{
			int proxifiedScriptCount = 0;
			struct dirent *entry;
			struct stat fileProperties;
			DIR* hScriptDir = opendir((this->env->cfg_dir+"/proxifiedScripts").c_str());
			while ((entry = readdir(hScriptDir))) {
				if (std::string(entry->d_name) != "." && std::string(entry->d_name) != ".."){
					proxifiedScriptCount++;
				}
			}
			closedir(hScriptDir);
			
			if (proxifiedScriptCount == 0)
				rmdir((this->env->cfg_dir+"/proxifiedScripts").c_str());
		}
	
		//add or remove proxy binary
		
		FILE* proxyBin = fopen((this->env->cfg_dir+"/bin/grubcfg_proxy").c_str(), "r");
		bool proxybin_exists = proxyBin != NULL;
		if (proxyBin) {
			fclose(proxyBin);
		}
		std::string dummyproxy_code = "#!/bin/sh\ncat\n";
		
		/**
		 * copy the grub customizer proxy, if required
		 */
		if (proxyCount != 0){
			// create the bin subdirectory - may already exist
			int bin_mk_success = mkdir((this->env->cfg_dir+"/bin").c_str(), 0755);
	
			FILE* proxyBinSource = fopen((std::string(LIBDIR)+"/grubcfg-proxy").c_str(), "r");
			
			if (proxyBinSource){
				FILE* proxyBinTarget = fopen((this->env->cfg_dir+"/bin/grubcfg_proxy").c_str(), "w");
				if (proxyBinTarget){
					int c;
					while ((c = fgetc(proxyBinSource)) != EOF){
						fputc(c, proxyBinTarget);
					}
					fclose(proxyBinTarget);
					chmod((this->env->cfg_dir+"/bin/grubcfg_proxy").c_str(), 0755);
				} else {
					this->log("could not open proxy output file!", Logger::ERROR);
				}
				fclose(proxyBinSource);
			} else {
				this->log("proxy could not be copied, generating dummy!", Logger::ERROR);
				FILE* proxyBinTarget = fopen((this->env->cfg_dir+"/bin/grubcfg_proxy").c_str(), "w");
				if (proxyBinTarget){
					fputs(dummyproxy_code.c_str(), proxyBinTarget);
					error_proxy_not_found = true;
					fclose(proxyBinTarget);
					chmod((this->env->cfg_dir+"/bin/grubcfg_proxy").c_str(), 0755);
				} else {
					this->log("coundn't create proxy!", Logger::ERROR);
				}
			}
		}
		else if (proxyCount == 0 && proxybin_exists){
			//the following commands are only cleanup… no problem, when they fail
			unlink((this->env->cfg_dir+"/bin/grubcfg_proxy").c_str());
			rmdir((this->env->cfg_dir+"/bin").c_str());
		}
	
		//update modified "custom" scripts
		for (auto script : this->repository) {
			if (script->isCustomScript && script->isModified()) {
				this->log("modifying script \"" + script->name + "\"", Logger::INFO);
				assert(script->fileName != "");
				auto dummyProxy = std::make_shared<Model_Proxy>(script);
				std::ofstream scriptStream(script->fileName.c_str());
				scriptStream << CUSTOM_SCRIPT_SHEBANG << "\n" << CUSTOM_SCRIPT_PREFIX << "\n";
				for (auto rule : dummyProxy->rules) {
					rule->print(scriptStream);
					if (rule->dataSource) {
						rule->dataSource->isModified = false;
					}
				}
			}
		}
	
	
		int saveProcSuccess = 0;
		std::string saveProcOutput;
	
		//run update-grub
		FILE* saveProc = popen((env->update_cmd + " 2>&1").c_str(), "r");
		if (saveProc) {
			int c;
			std::string row = "";
			while ((c = fgetc(saveProc)) != EOF) {
				saveProcOutput += char(c);
				if (c == '\n') {
					send_new_save_progress(0.5); //a gui should use pulse() instead of set_fraction
					this->log(row, Logger::INFO);
					row = "";
				} else {
					row += char(c);
				}
			}
			saveProcSuccess = pclose(saveProc);
		}
	
		// correct pathes of foreign rules (to make sure re-syncing works)
		auto foreignRules = this->proxies.getForeignRules();
		for (auto foreignRule : foreignRules) {
			auto entry = foreignRule->dataSource;
			assert(entry != nullptr);
			auto script = this->repository.getScriptByEntry(entry);
			assert(script != nullptr);
			foreignRule->__sourceScriptPath = script->fileName;
		}
	
		send_new_save_progress(1);
	
		if ((saveProcSuccess != 0 || saveProcOutput.find("Syntax errors are detected in generated GRUB config file") != -1)){
			throw CmdExecException("failed running '" + env->update_cmd + "' output:\n" + saveProcOutput, __FILE__, __LINE__);
		}
	}

	public: void readGeneratedFile(FILE* source, bool createScriptIfNotFound = false, bool createProxyIfNotFound = false)
	{
		Model_Entry_Row row;
		std::shared_ptr<Model_Script> script = nullptr;
		int i = 0;
		bool inScript = false;
		std::string plaintextBuffer = "";
		int innerCount = 0;
		double progressbarScriptSpace = 0.7 / this->repository.size();
		while (!cancelThreadsRequested && (row = Model_Entry_Row(source))){
			std::string rowText = Helper::ltrim(row.text);
			if (!inScript && rowText.substr(0,10) == ("### BEGIN ") && rowText.substr(rowText.length()-4,4) == " ###"){
				this->lock();
				if (script) {
					if (plaintextBuffer != "" && !script->isModified()) {
						auto newEntry = std::make_shared<Model_Entry>("#text", "", plaintextBuffer, Model_Entry::PLAINTEXT);
						if (this->hasLogger()) {
							newEntry->setLogger(this->getLogger());
						}
						script->entries().push_front(newEntry);
					}
					this->proxies.sync_all(true, true, script);
				}
				plaintextBuffer = "";
				std::string scriptName = rowText.substr(10, rowText.length()-14);
				std::string prefix = this->env->cfg_dir_prefix;
				std::string realScriptName = prefix+scriptName;
				if (realScriptName.substr(0, (this->env->cfg_dir+"/LS_").length()) == this->env->cfg_dir+"/LS_"){
					realScriptName = prefix+readScriptForwarder(realScriptName);
				}
				script = repository.getScriptByFilename(realScriptName, createScriptIfNotFound);
				if (createScriptIfNotFound && createProxyIfNotFound){ //for the compare-configuration
					this->proxies.push_back(std::make_shared<Model_Proxy>(script));
				}
				this->unlock();
				if (script){
					this->send_new_load_progress(0.1 + (progressbarScriptSpace * ++i + (progressbarScriptSpace/10*innerCount)), script->name, i, this->repository.size());
				}
				inScript = true;
			} else if (inScript && rowText.substr(0,8) == ("### END ") && rowText.substr(rowText.length()-4,4) == " ###") {
				inScript = false;
				innerCount = 0;
			} else if (script != nullptr && rowText.substr(0, 10) == "menuentry ") {
				this->lock();
				if (innerCount < 10) {
					innerCount++;
				}
				auto newEntry = std::make_shared<Model_Entry>(source, row, this->getLogger());
				if (!script->isModified()) {
					script->entries().push_back(newEntry);
				}
				this->proxies.sync_all(false, false, script);
				this->unlock();
				this->send_new_load_progress(0.1 + (progressbarScriptSpace * i + (progressbarScriptSpace/10*innerCount)), script->name, i, this->repository.size());
			} else if (script != NULL && rowText.substr(0, 8) == "submenu ") {
				this->lock();
				auto newEntry = std::make_shared<Model_Entry>(source, row, this->getLogger());
				script->entries().push_back(newEntry);
				this->proxies.sync_all(false, false, script);
				this->unlock();
				this->send_new_load_progress(0.1 + (progressbarScriptSpace * i + (progressbarScriptSpace/10*innerCount)), script->name, i, this->repository.size());
			} else if (inScript) { //Plaintext
				plaintextBuffer += row.text + "\n";
			}
		}
		this->lock();
		if (script) {
			if (plaintextBuffer != "" && !script->isModified()) {
				auto newEntry = std::make_shared<Model_Entry>("#text", "", plaintextBuffer, Model_Entry::PLAINTEXT);
				if (this->hasLogger()) {
					newEntry->setLogger(this->getLogger());
				}
				script->entries().push_front(newEntry);
			}
			this->proxies.sync_all(true, true, script);
		}
	
		// sync all (including foreign entries)
		this->proxies.sync_all(true, true, nullptr, this->repository.getScriptPathMap());
	
		this->unlock();
	}

	public: std::map<std::shared_ptr<Model_Entry>, std::shared_ptr<Model_Script>> getEntrySources(
		std::shared_ptr<Model_Proxy> proxy,
		std::shared_ptr<Model_Rule> parent = nullptr
	) {
		auto& list = parent ? parent->subRules : proxy->rules;

		std::map<std::shared_ptr<Model_Entry>, std::shared_ptr<Model_Script>> result;
		assert(proxy->dataSource != nullptr);

		for (auto rule : list) {
			if (rule->dataSource && !proxy->ruleIsFromOwnScript(rule)) {
				auto script = this->repository.getScriptByEntry(rule->dataSource);
				if (script != nullptr) {
					result[rule->dataSource] = script;
				} else {
					this->log("error finding the associated script! (" + rule->outputName + ")", Logger::WARNING);
				}
			} else if (rule->type == Model_Rule::SUBMENU) {
				auto subResult = this->getEntrySources(proxy, rule);
				if (subResult.size()) {
					result.insert(subResult.begin(), subResult.end());
				}
			}
		}
		return result;
	}

	public: bool loadStaticCfg()
	{
		FILE* oldConfigFile = fopen(env->output_config_file.c_str(), "r");
		if (oldConfigFile){
			this->readGeneratedFile(oldConfigFile, true, true);
			fclose(oldConfigFile);
			return true;
		}
		return false;
	}


	public: void send_new_load_progress(double newProgress, std::string scriptName = "", int current = 0, int max = 0)
	{
		if (this->onLoadStateChange){
			this->progress = newProgress;
			this->progress_name = scriptName;
			this->progress_pos = current;
			this->progress_max = max;
			this->onLoadStateChange();
		} else if (this->verbose) {
			this->log("cannot show updated load progress - no event handler assigned!", Logger::ERROR);
		}
	}

	public: void send_new_save_progress(double newProgress)
	{
		if (this->onSaveStateChange){
			this->progress = newProgress;
			this->onSaveStateChange();
		} else if (this->verbose) {
			this->log("cannot show updated save progress - no event handler assigned!", Logger::ERROR);
		}
	}

	public: void cancelThreads()
	{
		cancelThreadsRequested = true;
	}

	public: void reset()
	{
		this->lock();
		this->repository.clear();
		this->repository.trash.clear();
		this->proxies.clear();
		this->proxies.trash.clear();
		this->unlock();
	}

	public: double getProgress() const
	{
		return progress;
	}

	public: std::string getProgress_name() const
	{
		return progress_name;
	}

	public: int getProgress_pos() const
	{
		return progress_pos;
	}

	public: int getProgress_max() const
	{
		return progress_max;
	}

	public: void renumerate(bool favorDefaultOrder = true)
	{
		short int i = 0;
		for (auto proxy : this->proxies) {
			bool isDefaultNumber = false;
			if (favorDefaultOrder && proxy->dataSource) {
				std::string sourceFileName = this->scriptSourceMap.getSourceName(proxy->dataSource->fileName);
				try {
					int prefixNum = Model_Script::extractIndexFromPath(sourceFileName, this->env->cfg_dir);
					if (prefixNum >= i) {
						i = prefixNum;
						isDefaultNumber = true;
					}
				} catch (InvalidStringFormatException const& e) {
					this->log(e, Logger::ERROR);
				}
			}
	
			bool retry = false;
			do {
				retry = false;
	
				proxy->index = i;
	
				if (!isDefaultNumber && proxy->dataSource) {
					// make sure that scripts never get a filePath that matches a script source (unless it's the source script)
					std::ostringstream fullFileName;
					fullFileName << this->env->cfg_dir << "/" << std::setw(2) << std::setfill('0') << i << "_" << proxy->dataSource->name;
					if (this->scriptSourceMap.has(fullFileName.str())) {
						i++;
						retry = true;
					}
				}
			} while (retry);
	
			i++;
		}
		this->proxies.sort();
	
		if (favorDefaultOrder && i > 100) { // if positions are out of range...
			this->renumerate(false); // retry without favorDefaultOrder
		}
	}

	public: std::shared_ptr<Model_Rule> createSubmenu(std::shared_ptr<Model_Rule> position)
	{
		return this->proxies.getProxyByRule(position)->createSubmenu(position);
	}

	public: std::shared_ptr<Model_Rule> splitSubmenu(std::shared_ptr<Model_Rule> child)
	{
		return this->proxies.getProxyByRule(child)->splitSubmenu(child);
	}

	public: bool cfgDirIsClean()
	{
		DIR* hGrubCfgDir = opendir(this->env->cfg_dir.c_str());
		if (hGrubCfgDir){
			struct dirent *entry;
			struct stat fileProperties;
			while ((entry = readdir(hGrubCfgDir))){
				std::string fname = entry->d_name;
				if ((fname.length() >= 4 && fname.substr(0,3) == "LS_") || fname.substr(0,3) == "PS_" || fname.substr(0,3) == "DS_")
					return false;
			}
			closedir(hGrubCfgDir);
		}
		return true;
	}

	public: void cleanupCfgDir()
	{
		this->log("cleaning up cfg dir!", Logger::IMPORTANT_EVENT);
		
		DIR* hGrubCfgDir = opendir(this->env->cfg_dir.c_str());
		if (hGrubCfgDir){
			struct dirent *entry;
			struct stat fileProperties;
			std::list<std::string> lsfiles, dsfiles, psfiles;
			std::list<std::string> proxyscripts;
			while ((entry = readdir(hGrubCfgDir))){
				std::string fname = entry->d_name;
				if (fname.length() >= 4){
					if (fname.substr(0,3) == "LS_")
						lsfiles.push_back(fname);
					else if (fname.substr(0,3) == "DS_")
						dsfiles.push_back(fname);
					else if (fname.substr(0,3) == "PS_")
						psfiles.push_back(fname);
	
					else if (fname[0] >= '1' && fname[0] <= '9' && fname[1] >= '0' && fname[1] <= '9' && fname[2] == '_')
						proxyscripts.push_back(fname);
				}
			}
			closedir(hGrubCfgDir);
			
			for (std::list<std::string>::iterator iter = lsfiles.begin(); iter != lsfiles.end(); iter++){
				this->log("deleting " + *iter, Logger::EVENT);
				unlink((this->env->cfg_dir+"/"+(*iter)).c_str());
			}
			//proxyscripts will be disabled before loading the config. While the provious mode will only be saved on the objects, every script should be made executable
			for (std::list<std::string>::iterator iter = proxyscripts.begin(); iter != proxyscripts.end(); iter++){
				this->log("re-activating " + *iter, Logger::EVENT);
				chmod((this->env->cfg_dir+"/"+(*iter)).c_str(), 0755);
			}
	
			//remove the DS_ prefix  (DS_10_foo -> 10_foo)
			for (std::list<std::string>::iterator iter = dsfiles.begin(); iter != dsfiles.end(); iter++) {
				this->log("renaming " + *iter, Logger::EVENT);
				std::string newPath = this->env->cfg_dir+"/"+iter->substr(3);
				Helper::assert_filepath_empty(newPath, __FILE__, __LINE__);
				rename((this->env->cfg_dir+"/"+(*iter)).c_str(), newPath.c_str());
			}
	
			//remove the PS_ prefix and add index prefix (PS_foo -> 10_foo)
			int i = 20; //prefix
			for (std::list<std::string>::iterator iter = psfiles.begin(); iter != psfiles.end(); iter++) {
				this->log("renaming " + *iter, Logger::EVENT);
				std::string out = *iter;
				out.replace(0, 2, (std::string("") + char('0' + (i/10)%10) + char('0' + i%10)));
				std::string newPath = this->env->cfg_dir+"/"+out;
				Helper::assert_filepath_empty(newPath, __FILE__, __LINE__);
				rename((this->env->cfg_dir+"/"+(*iter)).c_str(), newPath.c_str());
				i++;
			}
		}
	}

	
	public: bool compare(Model_ListCfg const& other) const
	{
		std::array<std::list<std::shared_ptr<Model_Rule>>, 2> rlist;
		for (int i = 0; i < 2; i++){
			const Model_ListCfg* gc = i == 0 ? this : &other;
			for (auto proxy : gc->proxies) {
				assert(proxy->dataSource != nullptr);
				if (proxy->isExecutable() && proxy->dataSource){
					if (proxy->dataSource->fileName == "") { // if the associated file isn't found
						return false;
					}
					std::string fname = proxy->dataSource->fileName.substr(other.env->cfg_dir.length()+1);
					if (i == 0 || (fname[0] >= '1' && fname[0] <= '9' && fname[1] >= '0' && fname[1] <= '9' && fname[2] == '_')) {
						auto comparableRules = this->getComparableRules(proxy->rules);
						rlist[i].splice(rlist[i].end(), comparableRules);
					}
				}
			}
		}
		return Model_ListCfg::compareLists(rlist[0], rlist[1]);
	}

	public: static std::list<std::shared_ptr<Model_Rule>> getComparableRules(std::list<std::shared_ptr<Model_Rule>> const& list)
	{
		std::list<std::shared_ptr<Model_Rule>> result;
		for (auto rule : list) {
			if (((rule->type == Model_Rule::NORMAL && rule->dataSource) || (rule->type == Model_Rule::SUBMENU && rule->hasRealSubrules())) && rule->isVisible){
				result.push_back(rule);
			}
		}
		return result;
	}

	public: static bool compareLists(std::list<std::shared_ptr<Model_Rule>> a, std::list<std::shared_ptr<Model_Rule>> b)
	{
		if (a.size() != b.size()) {
			return false;
		}
	
		auto self_iter = a.begin(), other_iter = b.begin();
		while (self_iter != a.end() && other_iter != b.end()){
			if ((*self_iter)->type != (*other_iter)->type) {
				return false;
			}
			assert((*self_iter)->type == (*other_iter)->type);
			//check this Rule
			if ((*self_iter)->outputName != (*other_iter)->outputName)
				return false;
			if ((*self_iter)->dataSource) {
				if ((*self_iter)->dataSource->extension != (*other_iter)->dataSource->extension)
					return false;
				if ((*self_iter)->dataSource->content != (*other_iter)->dataSource->content)
					return false;
				if ((*self_iter)->dataSource->type != (*other_iter)->dataSource->type)
					return false;
			}
			//check rules inside the submenu
			if ((*self_iter)->type == Model_Rule::SUBMENU && !Model_ListCfg::compareLists(Model_ListCfg::getComparableRules((*self_iter)->subRules), Model_ListCfg::getComparableRules((*other_iter)->subRules))) {
				return false;
			}
			self_iter++;
			other_iter++;
		}
		return true;
	}


	public: void renameRule(std::shared_ptr<Model_Rule> rule, std::string const& newName)
	{
		rule->outputName = newName;
	}

	public: std::string getRulePath(std::shared_ptr<Model_Rule> rule)
	{
		auto proxy = this->proxies.getProxyByRule(rule);
		std::stack<std::string> ruleNameStack;
		ruleNameStack.push(rule->outputName);
	
		auto currentRule = rule;
		while ((currentRule = proxy->getParentRule(currentRule))) {
			ruleNameStack.push(currentRule->outputName);
		}
	
		std::string output = ruleNameStack.top();
		ruleNameStack.pop();
		while (ruleNameStack.size()) {
			output += ">" + ruleNameStack.top();
			ruleNameStack.pop();
		}
		return output;
	}

	public: std::string getGrubErrorMessage() const
	{
		FILE* errorLogFile = fopen(this->errorLogFile.c_str(), "r");
		std::string errorMessage;
		int c;
		while ((c = fgetc(errorLogFile)) != EOF) {
			errorMessage += char(c);
		}
		fclose(errorLogFile);
		return errorMessage;
	}


	public: void addColorHelper()
	{
		if (this->repository.getScriptByName("grub-customizer_menu_color_helper") == nullptr) {
			std::shared_ptr<Model_Script> newScript = this->repository.createScript("grub-customizer_menu_color_helper", this->env->cfg_dir + "06_grub-customizer_menu_color_helper", "#!/bin/sh\n\
	\n\
	if [ \"x${GRUB_BACKGROUND}\" != \"x\" ] ; then\n\
		if [ \"x${GRUB_COLOR_NORMAL}\" != \"x\" ] ; then\n\
		echo \"set color_normal=${GRUB_COLOR_NORMAL}\"\n\
		fi\n\
	\n\
		if [ \"x${GRUB_COLOR_HIGHLIGHT}\" != \"x\" ] ; then\n\
		echo \"set color_highlight=${GRUB_COLOR_HIGHLIGHT}\"\n\
		fi\n\
	fi\n\
	");
			assert(newScript != nullptr);
			auto newProxy = std::make_shared<Model_Proxy>(newScript);
			newProxy->index = 6;
			this->proxies.push_back(newProxy);
		}
	}


	public: std::list<std::shared_ptr<Model_Rule>> getRemovedEntries(
		std::shared_ptr<Model_Entry> parent = nullptr,
		bool ignorePlaceholders = false
	) {
		std::list<std::shared_ptr<Model_Rule>> result;
		if (parent == nullptr) {
			for (auto script : this->repository) {
				auto subResult = this->getRemovedEntries(script->root, ignorePlaceholders);
				result.insert(result.end(), subResult.begin(), subResult.end());
			}
		} else {
			if (parent->type == Model_Entry::SUBMENU || parent->type == Model_Entry::SCRIPT_ROOT) {
				for (auto entry : parent->subEntries) {
					auto subResult = this->getRemovedEntries(entry, ignorePlaceholders);
					std::shared_ptr<Model_Rule> currentSubmenu = nullptr;
					if (subResult.size()) {
						auto submenu = std::make_shared<Model_Rule>(Model_Rule::SUBMENU, std::list<std::string>(), entry->name, true);
						submenu->subRules = subResult;
						submenu->dataSource = entry;
						result.push_back(submenu);
						currentSubmenu = result.back();
					}
	
					if ((entry->type == Model_Entry::MENUENTRY || !ignorePlaceholders) && !this->proxies.getVisibleRuleForEntry(entry)) {
						Model_Rule::RuleType ruleType = Model_Rule::NORMAL;
						switch (entry->type) {
						case Model_Entry::MENUENTRY:
							ruleType = Model_Rule::NORMAL;
							break;
						case Model_Entry::PLAINTEXT:
							ruleType = Model_Rule::PLAINTEXT;
							break;
						case Model_Entry::SUBMENU:
							ruleType = Model_Rule::OTHER_ENTRIES_PLACEHOLDER;
							break;
						}
						auto newRule = std::make_shared<Model_Rule>(ruleType, std::list<std::string>(), entry->name, true);
						newRule->dataSource = entry;
						if (currentSubmenu) {
							currentSubmenu->subRules.push_front(newRule);
						} else {
							result.push_back(newRule);
						}
					}
				}
			}
		}
		return result;
	}

	public: std::shared_ptr<Model_Rule> addEntry(
		std::shared_ptr<Model_Entry> entry,
		bool insertAsOtherEntriesPlaceholder = false
	) {
		auto sourceScript = this->repository.getScriptByEntry(entry);
		assert(sourceScript != nullptr);
	
		std::shared_ptr<Model_Proxy> targetProxy = nullptr;
		if (this->proxies.size() && this->proxies.back()->dataSource == sourceScript) {
			targetProxy = this->proxies.back();
			targetProxy->set_isExecutable(true);
		} else {
			this->proxies.push_back(std::make_shared<Model_Proxy>(sourceScript, false));
			targetProxy = this->proxies.back();
			this->renumerate();
		}
	
		std::shared_ptr<Model_Rule> rule = nullptr;
		if (insertAsOtherEntriesPlaceholder) {
			rule = std::make_shared<Model_Rule>(Model_Rule::OTHER_ENTRIES_PLACEHOLDER, sourceScript->buildPath(entry), true);
			rule->dataSource = entry;
		} else {
			rule = std::make_shared<Model_Rule>(
				entry,
				true,
				sourceScript,
				std::list<std::list<std::string>>(),
				sourceScript->buildPath(entry)
			);
		}
	
		targetProxy->removeEquivalentRules(rule);
		targetProxy->rules.push_back(rule);
		return targetProxy->rules.back();
	}


	public: void deleteEntry(std::shared_ptr<Model_Entry> entry)
	{
		for (auto proxy : this->proxies) {
			auto rule = proxy->getRuleByEntry(entry, proxy->rules, Model_Rule::NORMAL);
			if (rule) {
				proxy->removeRule(rule);
			}
		}
		this->repository.getScriptByEntry(entry)->deleteEntry(entry);
	}


	public: std::list<Rule*> getNormalizedRuleOrder(std::list<Rule*> rules)
	{
		if (rules.size() == 0 || rules.size() == 1) {
			return rules;
		}
		std::list<Rule*> result;
	
		auto firstRuleOfList = this->findRule(rules.front());
		std::list<std::shared_ptr<Model_Rule>>::iterator currentRule;
	
		auto parentRule = this->proxies.getProxyByRule(firstRuleOfList)->getParentRule(firstRuleOfList);
		if (parentRule) {
			currentRule = parentRule->subRules.begin();
		} else {
			currentRule = this->proxies.front()->rules.begin();
		}
	
		try {
			while (true) {
				for (auto rulePtr : rules) {
					if (currentRule->get() == rulePtr) {
						result.push_back(rulePtr);
						break;
					}
				}
				currentRule = this->proxies.getNextVisibleRule(currentRule, 1);
			}
		} catch (NoMoveTargetException const& e) {
			// loop finished
		}
	
		return result;
	}


	public: std::list<std::shared_ptr<Model_Script>> getProxifiedScripts()
	{
		std::list<std::shared_ptr<Model_Script>> result;
	
		for (auto script : this->repository) {
			if (this->proxies.proxyRequired(script)) {
				result.push_back(script);
			}
		}
	
		return result;
	}

	public: void generateScriptSourceMap()
	{
		std::map<std::string, int> defaultScripts; // only for non-static scripts - so 40_custom is ignored
		defaultScripts["header"]                            =  0;
		defaultScripts["debian_theme"]                      =  5;
		defaultScripts["grub-customizer_menu_color_helper"] =  6;
		defaultScripts["linux"]                             = 10;
		defaultScripts["linux_xen"]                         = 20;
		defaultScripts["memtest86+"]                        = 20;
		defaultScripts["os-prober"]                         = 30;
		defaultScripts["custom"]                            = 41;
	
		std::string proxyfiedScriptPath = this->env->cfg_dir + "/proxifiedScripts";
	
		for (auto script : this->repository) {
			std::string currentPath = script->fileName;
			std::string defaultPath;
			int pos = -1;
	
			if (script->isCustomScript) {
				defaultPath = this->env->cfg_dir + "/40_custom";
			} else if (defaultScripts.find(script->name) != defaultScripts.end()) {
				pos = defaultScripts[script->name];
				std::ostringstream str;
				str << this->env->cfg_dir << "/" << std::setw(2) << std::setfill('0') << pos << "_" << script->name;
				defaultPath = str.str();
			}
	
			if (defaultPath != "" && defaultPath != currentPath) {
				this->scriptSourceMap[defaultPath] = currentPath;
			}
		}
	}

	public: void populateScriptSourceMap()
	{
		std::string proxyfiedScriptPath = this->env->cfg_dir + "/proxifiedScripts";
		for (auto script : this->repository) {
			if (script->fileName.substr(0, proxyfiedScriptPath.length()) != proxyfiedScriptPath
					&& this->scriptSourceMap.getSourceName(script->fileName) == "") {
				this->scriptSourceMap.addScript(script->fileName);
			}
		}
	}

	public: bool hasScriptUpdates() const
	{
		return this->scriptSourceMap.getUpdates().size() > 0;
	}

	public: void applyScriptUpdates()
	{
		std::list<std::string> newScriptPathes = this->scriptSourceMap.getUpdates();
		for (auto newScriptPath : newScriptPathes) {
			std::string oldScriptPath = this->scriptSourceMap[newScriptPath];
			auto oldScript = this->repository.getScriptByFilename(oldScriptPath);
			auto newScript = this->repository.getScriptByFilename(newScriptPath);
			if (!oldScript || !newScript) {
				this->log("applyScriptUpdates failed for " + oldScriptPath + " (" + newScriptPath + ")", Logger::ERROR);
				continue;
			}
	
			// unsync proxies of newScript
			auto newProxies = this->proxies.getProxiesByScript(newScript);
			for (auto newProxy : newProxies) {
				newProxy->unsync();
				this->proxies.deleteProxy(newProxy);
			}
	
			// copy entries of custom scripts
			if (oldScript->isCustomScript && newScript->isCustomScript && oldScript->entries().size()) {
				for (auto entry : oldScript->entries()) {
					if (entry->type == Model_Entry::PLAINTEXT && newScript->getPlaintextEntry()) {
						newScript->getPlaintextEntry()->content = entry->content; // copy plaintext instead of adding another entry
						newScript->getPlaintextEntry()->isModified = true;
					} else {
						newScript->entries().push_back(entry);
						newScript->entries().back()->isModified = true;
					}
				}
			}
	
			// connect proxies of oldScript with newScript, resync
			auto oldProxies = this->proxies.getProxiesByScript(oldScript);
			for (auto oldProxy : oldProxies) {
				// connect old proxy to new script
				oldProxy->unsync();
				oldProxy->dataSource = newScript;
				if (oldProxy->fileName == oldScript->fileName) {
					oldProxy->fileName = newScript->fileName; // set the new fileName
				}
			}
	
			auto foreignRules = this->proxies.getForeignRules();
	
			for (auto rule : foreignRules) {
				if (this->repository.getScriptByEntry(rule->dataSource) == oldScript) {
					rule->__sourceScriptPath = newScript->fileName;
				}
			}
	
			this->repository.removeScript(oldScript);
		}
		this->scriptSourceMap.deleteUpdates();
	
		this->proxies.unsync_all();
		this->proxies.sync_all(true, true, nullptr, this->repository.getScriptPathMap());
	}


	public: void revert()
	{
		int remaining = this->proxies.size();
		while (remaining) {
			this->proxies.deleteProxy(this->proxies.front());
			assert(this->proxies.size() < remaining); // make sure that the proxy has really been deleted to prevent an endless loop
			remaining = this->proxies.size();
		}
		std::list<std::string> usedIndices;
		int i = 50; // unknown scripts starting at position 50
		for (auto script : this->repository) {
			auto newProxy = std::make_shared<Model_Proxy>(script);
			std::string sourceFileName = this->scriptSourceMap.getSourceName(script->fileName);
			try {
				newProxy->index = Model_Script::extractIndexFromPath(sourceFileName, this->env->cfg_dir);
			} catch (InvalidStringFormatException const& e) {
				newProxy->index = i++;
				this->log(e, Logger::ERROR);
			}
	
			// avoid duplicates
			std::ostringstream uniqueIndex;
			uniqueIndex << newProxy->index << script->name;
	
			if (std::find(usedIndices.begin(), usedIndices.end(), uniqueIndex.str()) != usedIndices.end()) {
				newProxy->index = i++;
			}
	
			usedIndices.push_back(uniqueIndex.str());
	
			this->proxies.push_back(newProxy);
		}
		this->proxies.sort();
	}

	public: std::shared_ptr<Model_Rule> findRule(Rule const* rulePtr)
	{
		auto allProxies = this->proxies;
		allProxies.insert(allProxies.end(), this->proxies.trash.begin(), this->proxies.trash.end());

		for (auto proxy : allProxies) {
			auto result = this->findRule(rulePtr, proxy->rules);
			if (result != nullptr) {
				return result;
			}
		}

		throw ItemNotFoundException("rule not found", __FILE__, __LINE__);
	}

	private: std::shared_ptr<Model_Rule> findRule(Rule const* rulePtr, std::list<std::shared_ptr<Model_Rule>> list)
	{
		for (auto rule : list) {
			if (rule.get() == rulePtr) {
				return rule;
			}

			auto subResult = this->findRule(rulePtr, rule->subRules);
			if (subResult != nullptr) {
				return subResult;
			}
		}
		return nullptr;
	}


	public: operator ArrayStructure() const  {
		ArrayStructure result;
		result["proxies"] = ArrayStructure(this->proxies);
		result["repository"] = ArrayStructure(this->repository);
		result["progress"] = this->progress;
		result["progress_name"] = this->progress_name;
		result["progress_pos"] = this->progress_pos;
		result["progress_max"] = this->progress_max;
		result["errorLogFile"] = this->errorLogFile;
		result["verbose"] = this->verbose;
		result["error_proxy_not_found"] = this->error_proxy_not_found;
		if (this->env) {
			result["env"] = ArrayStructure(*this->env);
		} else {
			result["env"] = ArrayStructureItem(NULL);
		}
		result["ignoreLock"] = this->ignoreLock;
		result["cancelThreadsRequested"] = this->cancelThreadsRequested;
		return result;
	}
};

class Model_ListCfg_Connection
{
	protected: std::shared_ptr<Model_ListCfg> grublistCfg;

	public:	virtual ~Model_ListCfg_Connection(){}

	public: void setListCfg(std::shared_ptr<Model_ListCfg> grublistCfg)
	{
		this->grublistCfg = grublistCfg;

		this->initListCfgEvents();
	}

	public: virtual void initListCfgEvents()
	{
		// override to initialize specific view events
	}
};


#endif
