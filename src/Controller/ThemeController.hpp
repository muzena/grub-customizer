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

#ifndef THEMECONTROLLERIMPL_H_
#define THEMECONTROLLERIMPL_H_
#include "../Model/Env.hpp"
#include "../View/Theme.hpp"
#include "../View/Trait/ViewAware.hpp"
#include "../Model/SettingsManagerData.hpp"
#include "../Model/ListCfg.hpp"
#include <algorithm>
#include <functional>

#include "../Model/ThemeManager.hpp"
#include "Common/ControllerAbstract.hpp"
#include "Helper/Thread.hpp"

class ThemeController :
	public Controller_Common_ControllerAbstract,
	public View_Trait_ViewAware<View_Theme>,
	public Model_ThemeManager_Connection,
	public Model_SettingsManagerData_Connection,
	public Model_ListCfg_Connection,
	public Model_Env_Connection,
	public Controller_Helper_Thread_Connection,
	public Bootstrap_Application_Object_Connection
{
	private: std::string currentTheme, currentThemeFile;
	private: bool syncActive; // should only be controlled by syncSettings()
	

	public: ThemeController() :
		Controller_Common_ControllerAbstract("theme"),
		syncActive(false)
	{
	}

	public: void initViewEvents() override
	{
		using namespace std::placeholders;

		this->view->onThemeSelected = std::bind(std::mem_fn(&ThemeController::loadThemeAction), this, _1);
		this->view->onThemeFileApply = std::bind(std::mem_fn(&ThemeController::addThemePackageAction), this, _1);
		this->view->onRemoveThemeClicked = std::bind(std::mem_fn(&ThemeController::removeThemeAction), this, _1);
		this->view->onAddThemeClicked = std::bind(std::mem_fn(&ThemeController::showThemeInstallerAction), this);
		this->view->onSimpleThemeSelected = std::bind(std::mem_fn(&ThemeController::showSimpleThemeConfigAction), this);
		this->view->onAddFile = std::bind(std::mem_fn(&ThemeController::addFileAction), this);
		this->view->onRemoveFile = std::bind(std::mem_fn(&ThemeController::removeFileAction), this, _1);
		this->view->onSelect = std::bind(std::mem_fn(&ThemeController::updateEditAreaAction), this, _1);
		this->view->onRename = std::bind(std::mem_fn(&ThemeController::renameAction), this, _1);
		this->view->onFileChoose = std::bind(std::mem_fn(&ThemeController::loadFileAction), this, _1);
		this->view->onTextChange = std::bind(std::mem_fn(&ThemeController::saveTextAction), this, _1);
		this->view->onColorChange = std::bind(std::mem_fn(&ThemeController::updateColorSettingsAction), this);
		this->view->onFontChange = std::bind(std::mem_fn(&ThemeController::updateFontSettingsAction), this, _1);
		this->view->onImageChange = std::bind(std::mem_fn(&ThemeController::updateBackgroundImageAction), this);
		this->view->onImageRemove = std::bind(std::mem_fn(&ThemeController::removeBackgroundImageAction), this);
		this->view->onSaveClick = std::bind(std::mem_fn(&ThemeController::saveAction), this);
	}

	public: void initApplicationEvents() override
	{
		using namespace std::placeholders;

		this->applicationObject->onListModelChange.addHandler(std::bind(std::mem_fn(&ThemeController::updateSettingsDataAction), this));

		this->applicationObject->onLoad.addHandler(std::bind(std::mem_fn(&ThemeController::loadThemesAction), this));
		this->applicationObject->onSettingModelChange.addHandler(std::bind(std::mem_fn(&ThemeController::updateFontSizeAction), this));

		this->applicationObject->onSave.addHandler(std::bind(std::mem_fn(&ThemeController::saveAction), this));
	}


	public: void loadThemesAction()
	{
		this->logActionBegin("load-themes");
		try {
			try {
				this->themeManager->load();
			} catch (FileReadException const& e) {
				this->log("Theme directory not found", Logger::INFO);
			}
			this->syncSettings();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void loadThemeAction(std::string const& name)
	{
		this->logActionBegin("load-theme");
		try {
			this->view->setEditorType(View_Theme::EDITORTYPE_THEME);
			this->view->setRemoveFunctionalityEnabled(true);
			this->view->selectTheme(name);
			this->currentTheme = name;
			try {
				this->themeManager->getTheme(name).getFileByNewName("theme.txt");
			} catch (ItemNotFoundException const& e) {
				this->view->showError(View_Theme::ERROR_THEMEFILE_NOT_FOUND);
			}
			this->settings->setValue("GRUB_THEME", this->themeManager->getThemePath() + "/" + name + "/theme.txt");
			this->settings->setIsActive("GRUB_THEME", true);
	
			this->syncFiles();
			this->syncSettings();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void addThemePackageAction(const std::string& filePath)
	{
		this->logActionBegin("add-theme-package");
		try {
			try {
				std::string themeName = this->themeManager->addThemePackage(filePath);
				this->themeManager->getTheme(themeName).isModified = true;
				this->loadThemeAction(themeName);
				this->syncSettings();
				this->view->selectTheme(themeName);
			} catch (InvalidFileTypeException const& e) {
				this->view->showError(View_Theme::ERROR_INVALID_THEME_PACK_FORMAT);
			}
		} catch (const Exception& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void removeThemeAction(const std::string& name)
	{
		this->logActionBegin("remove-theme");
		try {
			this->themeManager->removeTheme(this->themeManager->getTheme(name));
			this->showSimpleThemeConfigAction();
			this->syncSettings();
		} catch (const Exception& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void showThemeInstallerAction()
	{
		this->logActionBegin("show-theme-installer");
		try {
			this->view->showThemeFileChooser();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void showSimpleThemeConfigAction()
	{
		this->logActionBegin("show-simple-theme-config");
		try {
			this->view->setEditorType(View_Theme::EDITORTYPE_CUSTOM);
			this->view->setRemoveFunctionalityEnabled(false);
			this->view->selectTheme("");
			this->updateColorSettingsAction();
			this->settings->setIsActive("GRUB_THEME", false);
			this->currentTheme = "";
			this->currentThemeFile = "";
			this->syncSettings();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void addFileAction()
	{
		this->logActionBegin("add-file");
		try {
			std::string defaultName = this->view->getDefaultName();
			Model_Theme* theme = &this->themeManager->getTheme(this->currentTheme);
			if (!theme->hasConflicts(defaultName)) {
				Model_ThemeFile newFile(defaultName, true);
				newFile.content = "";
				newFile.contentLoaded = true;
				theme->files.push_back(newFile);
				theme->isModified = true;
				theme->sort();
				this->syncFiles();
				this->threadHelper->runDelayed(
					std::bind(std::mem_fn(&ThemeController::startFileEditAction), this, defaultName),
					10
				);
			} else {
				this->view->showError(View_Theme::ERROR_RENAME_CONFLICT);
			}
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void startFileEditAction(std::string const& file)
	{
		this->logActionBegin("select-file");
		try {
			this->view->selectFile(file, true);
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void removeFileAction(std::string const& file)
	{
		this->logActionBegin("remove-file");
		try {
			Model_ThemeFile* fileObj = &this->themeManager->getTheme(this->currentTheme).getFileByNewName(file);
			this->themeManager->getTheme(this->currentTheme).removeFile(*fileObj);
			this->themeManager->getTheme(this->currentTheme).isModified = true;
			this->currentThemeFile = "";
			this->syncFiles();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateEditAreaAction(std::string const& file)
	{
		this->logActionBegin("update-edit-area");
		try {
			Model_Theme* theme = &this->themeManager->getTheme(this->currentTheme);
			Model_ThemeFile* themeFile = &theme->getFileByNewName(file);
			std::string originalFileName = themeFile->localFileName;
			bool isImage = this->isImage(file);
			this->currentThemeFile = themeFile->newLocalFileName;
			if (themeFile->content != "") {
				this->view->setText(themeFile->content);
			} else if (themeFile->externalSource != "") {
				if (isImage) {
					this->view->setImage(themeFile->externalSource);
				} else {
					std::string content = theme->loadFileContentExternal(themeFile->externalSource);
					this->view->setText(content);
				}
			} else if (!themeFile->isAddedByUser) {
				if (isImage) {
					this->view->setImage(theme->getFullFileName(originalFileName));
				} else {
					std::string content = theme->loadFileContent(originalFileName);
					this->view->setText(content);
				}
			} else {
				if (!isImage) {
					this->view->setText("");
				}
			}
			this->view->setCurrentExternalThemeFilePath(themeFile->externalSource);
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void renameAction(std::string const& newName)
	{
		this->logActionBegin("rename");
		try {
			Model_ThemeFile* themeFile = &this->themeManager->getTheme(this->currentTheme).getFile(this->currentThemeFile);
			if (themeFile->newLocalFileName == newName) {
				// do nothing
			} else if (!this->themeManager->getTheme(this->currentTheme).hasConflicts(newName)) {
				this->themeManager->getTheme(this->currentTheme).isModified = true;
				themeFile->newLocalFileName = newName;
				if (themeFile->isAddedByUser) {
					themeFile->localFileName = newName;
					this->currentThemeFile = newName;
				}
				this->updateEditAreaAction(newName);
			} else {
				this->view->showError(View_Theme::ERROR_RENAME_CONFLICT);
			}
	
			this->themeManager->getTheme(this->currentTheme).sort();
			this->syncFiles();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void loadFileAction(std::string const& externalPath)
	{
		this->logActionBegin("load-file");
		try {
			if (this->currentThemeFile == "") {
				this->view->showError(View_Theme::ERROR_NO_FILE_SELECTED);
				return;
			}
			this->themeManager->getTheme(this->currentTheme).isModified = true;
			Model_ThemeFile* file = &this->themeManager->getTheme(this->currentTheme).getFile(this->currentThemeFile);
			file->externalSource = externalPath;
			file->content = "";
			file->contentLoaded = false;
			this->updateEditAreaAction(file->newLocalFileName);
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void saveTextAction(std::string const& newText)
	{
		this->logActionBegin("save-text");
		try {
			this->themeManager->getTheme(this->currentTheme).isModified = true;
			Model_ThemeFile* themeFile = &this->themeManager->getTheme(this->currentTheme).getFile(this->currentThemeFile);
			themeFile->externalSource = "";
			this->view->setCurrentExternalThemeFilePath(themeFile->externalSource);
			themeFile->content = newText;
			themeFile->contentLoaded = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}


	public: void updateColorSettingsAction()
	{
		this->logActionBegin("update-color-settings");
		try {
			if (this->view->getColorChooser(View_Theme::COLOR_CHOOSER_DEFAULT_FONT).getSelectedColor() != "" && this->view->getColorChooser(View_Theme::COLOR_CHOOSER_DEFAULT_BACKGROUND).getSelectedColor() != ""){
				this->settings->setValue("GRUB_COLOR_NORMAL", this->view->getColorChooser(View_Theme::COLOR_CHOOSER_DEFAULT_FONT).getSelectedColor() + "/" + this->view->getColorChooser(View_Theme::COLOR_CHOOSER_DEFAULT_BACKGROUND).getSelectedColor());
				this->settings->setIsActive("GRUB_COLOR_NORMAL", true);
				this->settings->setIsExport("GRUB_COLOR_NORMAL", true);
			}
			if (this->view->getColorChooser(View_Theme::COLOR_CHOOSER_HIGHLIGHT_FONT).getSelectedColor() != "" && this->view->getColorChooser(View_Theme::COLOR_CHOOSER_HIGHLIGHT_BACKGROUND).getSelectedColor() != ""){
				this->settings->setValue("GRUB_COLOR_HIGHLIGHT", this->view->getColorChooser(View_Theme::COLOR_CHOOSER_HIGHLIGHT_FONT).getSelectedColor() + "/" + this->view->getColorChooser(View_Theme::COLOR_CHOOSER_HIGHLIGHT_BACKGROUND).getSelectedColor());
				this->settings->setIsActive("GRUB_COLOR_HIGHLIGHT", true);
				this->settings->setIsExport("GRUB_COLOR_HIGHLIGHT", true);
			}
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateFontSettingsAction(bool removeFont)
	{
		this->logActionBegin("update-font-settings");
		try {
			std::string fontName;
			int fontSize = -1;
	
			if (!removeFont) {
				fontName = this->view->getFontName();
				fontSize = this->view->getFontSize();
			}
			if (fontName != "" && this->settings->grubFont == "") {
				this->view->showFontWarning();
			}
	
			this->settings->grubFont = fontName;
			this->settings->grubFontSize = fontSize;
	
			if (fontName != "") {
				std::string fullTmpFontPath = this->env->cfg_dir_prefix + "/tmp/grub_customizer_chosen_font_test.pf2";
	
				this->settings->mkFont("", "/tmp/grub_customizer_chosen_font_test.pf2");
				this->settings->grubFont = this->settings->parsePf2(fullTmpFontPath)["NAME"];
	
				// must be done again to check the font returned by parsePf2
				this->settings->mkFont("", "/tmp/grub_customizer_chosen_font_test.pf2");
				this->settings->grubFont = this->settings->parsePf2(fullTmpFontPath)["NAME"];
	
				// to prevent usage of the temporary font file
				this->settings->removeItem("GRUB_FONT");
			}
	
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateFontSizeAction()
	{
		this->logActionBegin("update-font-size");
		try {
			this->settings->grubFontSize = this->view->getFontSize();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateBackgroundImageAction()
	{
		this->logActionBegin("update-background-image");
		try {
			if (!this->env->useDirectBackgroundProps) {
				this->settings->setValue("GRUB_MENU_PICTURE", this->view->getBackgroundImagePath());
				this->settings->setIsActive("GRUB_MENU_PICTURE", true);
				this->settings->setIsExport("GRUB_MENU_PICTURE", true);
			} else {
				this->settings->setValue("GRUB_BACKGROUND", this->view->getBackgroundImagePath());
				this->settings->setIsActive("GRUB_BACKGROUND", true);
			}
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void removeBackgroundImageAction()
	{
		this->logActionBegin("remove-background-image");
		try {
			if (!this->env->useDirectBackgroundProps) {
				this->settings->setIsActive("GRUB_MENU_PICTURE", false);
			} else {
				this->settings->setIsActive("GRUB_BACKGROUND", false);
			}
			this->syncSettings();
			this->env->modificationsUnsaved = true;
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void updateSettingsDataAction()
	{
		this->logActionBegin("update-settings-data");
		try {
			std::list<std::string> labelListToplevel  = this->grublistCfg->proxies.getToplevelEntryTitles();
	
			this->view->setPreviewEntryTitles(labelListToplevel);
	
			this->syncSettings();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}


	public: void syncAction()
	{
		this->logActionBegin("sync");
		try {
			this->syncSettings();
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void saveAction()
	{
		this->logActionBegin("save");
		try {
			this->themeManager->save();
			this->threadHelper->runDispatched(std::bind(std::mem_fn(&ThemeController::postSaveAction), this));
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	public: void postSaveAction()
	{
		this->logActionBegin("post-save");
		try {
			if (this->themeManager->hasSaveErrors()) {
				this->view->showError(View_Theme::ERROR_SAVE_FAILED, this->themeManager->getSaveErrors());
			}
			this->syncSettings();
			this->syncFiles();
			this->currentThemeFile = "";
			this->view->selectFile("");
		} catch (Exception const& e) {
			this->applicationObject->onError.exec(e);
		}
		this->logActionEnd();
	}

	private: bool isImage(std::string const& fileName)
	{
		std::list<std::string> imageExtensions;
		imageExtensions.push_back("png");
		imageExtensions.push_back("jpg");
		imageExtensions.push_back("bmp");
		imageExtensions.push_back("gif");
		imageExtensions.push_back("pf2"); // not really, but shouldn't be handled as text

		if (fileName.find_last_of(".") != std::string::npos) {
			int dotPos = fileName.find_last_of(".");
			std::string extension = fileName.substr(dotPos + 1);
			if (std::find(imageExtensions.begin(), imageExtensions.end(), extension) != imageExtensions.end()) {
				return true;
			}
		}
		return false;
	}

	private: void syncSettings()
	{
		if (this->syncActive) {
			return;
		}
		this->syncActive = true;
		std::string nColor = this->settings->getValue("GRUB_COLOR_NORMAL");
		std::string hColor = this->settings->getValue("GRUB_COLOR_HIGHLIGHT");
		if (nColor != ""){
			this->view->getColorChooser(View_Theme::COLOR_CHOOSER_DEFAULT_FONT).selectColor(nColor.substr(0, nColor.find('/')));
			this->view->getColorChooser(View_Theme::COLOR_CHOOSER_DEFAULT_BACKGROUND).selectColor(nColor.substr(nColor.find('/')+1));
		} else {
			//default grub menu colors
			this->view->getColorChooser(View_Theme::COLOR_CHOOSER_DEFAULT_FONT).selectColor("light-gray");
			this->view->getColorChooser(View_Theme::COLOR_CHOOSER_DEFAULT_BACKGROUND).selectColor("black");
		}

		if (hColor != ""){
			this->view->getColorChooser(View_Theme::COLOR_CHOOSER_HIGHLIGHT_FONT).selectColor(hColor.substr(0, hColor.find('/')));
			this->view->getColorChooser(View_Theme::COLOR_CHOOSER_HIGHLIGHT_BACKGROUND).selectColor(hColor.substr(hColor.find('/')+1));
		} else {
			//default grub menu colors
			this->view->getColorChooser(View_Theme::COLOR_CHOOSER_HIGHLIGHT_FONT).selectColor("magenta");
			this->view->getColorChooser(View_Theme::COLOR_CHOOSER_HIGHLIGHT_BACKGROUND).selectColor("black");
		}

		std::string wallpaper_key = this->env->useDirectBackgroundProps ? "GRUB_BACKGROUND" : "GRUB_MENU_PICTURE";
		std::string menuPicturePath = this->settings->getValue(wallpaper_key);
		bool menuPicIsInGrubDir = false;

		if (menuPicturePath != "" && menuPicturePath[0] != '/'){
			menuPicturePath = env->output_config_dir + "/" + menuPicturePath;
			menuPicIsInGrubDir = true;
		}

		this->view->setFontName(this->settings->grubFont);

		if (this->settings->isActive(wallpaper_key) && menuPicturePath != ""){
			this->view->setBackgroundImagePreviewPath(menuPicturePath, menuPicIsInGrubDir);
		} else {
			this->view->setBackgroundImagePreviewPath("", menuPicIsInGrubDir);
		}

		std::string selectedTheme = this->view->getSelectedTheme();

		this->view->clearThemeSelection();
		for (auto& theme : this->themeManager->themes) {
			this->view->addTheme(theme.name);
		}
		this->view->selectTheme(selectedTheme);

		std::string themeName;
		if (this->settings->isActive("GRUB_THEME")) {
			try {
				themeName = this->themeManager->extractThemeName(this->settings->getValue("GRUB_THEME"));
			} catch (InvalidStringFormatException const& e) {
				this->log(e, Logger::ERROR);
			}
		}

		if (this->currentTheme != themeName) {
			if (this->settings->isActive("GRUB_THEME")) {
				if (this->themeManager->themeExists(themeName)) {
					this->loadThemeAction(themeName);
				} else {
					this->log("theme " + themeName + " not found!", Logger::ERROR);
				}
			} else if (!this->settings->isActive("GRUB_THEME")) {
				this->showSimpleThemeConfigAction();
			}
		}

		this->applicationObject->onSettingModelChange.exec();
		this->syncActive = false;
	}

	private: void syncFiles()
	{
		this->view->clear();
		if (this->currentTheme != "") {
			Model_Theme* theme = &this->themeManager->getTheme(this->currentTheme);
			for (auto& themeFile : theme->files) {
				this->view->addFile(themeFile.newLocalFileName);
			}
			if (this->currentThemeFile != "") {
				this->view->selectFile(theme->getFile(this->currentThemeFile).newLocalFileName);
			}
		}
	}
};


#endif /* THEMECONTROLLERIMPL_H_ */
