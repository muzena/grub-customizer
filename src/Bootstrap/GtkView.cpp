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
#include "../View/Gtk/About.hpp"
#include "../View/Gtk/EntryEditor.hpp"
#include "../View/Gtk/EnvEditor.hpp"
#include "../View/Gtk/Error.hpp"
#include "../View/Gtk/Installer.hpp"
#include "../View/Gtk/Main.hpp"
#include "../View/Gtk/Settings.hpp"
#include "../View/Gtk/Theme.hpp"
#include "../View/Gtk/Trash.hpp"
#include "View.hpp"

Bootstrap_View::Bootstrap_View() {
	auto about = std::make_shared<View_Gtk_About>();
	auto entryEditor = std::make_shared<View_Gtk_EntryEditor>();
	auto envEditor = std::make_shared<View_Gtk_EnvEditor>();
	auto error = std::make_shared<View_Gtk_Error>();
	auto installer = std::make_shared<View_Gtk_Installer>();
	auto main = std::make_shared<View_Gtk_Main>();
	auto settings = std::make_shared<View_Gtk_Settings>();
	auto theme = std::make_shared<View_Gtk_Theme>();
	auto trash = std::make_shared<View_Gtk_Trash>();

	main->putSettingsDialog(settings->getCommonSettingsPane(), settings->getAppearanceSettingsPane());
	main->putTrashList(trash->getList());
	settings->putThemeSelector(theme->getThemeSelector());
	settings->putThemeEditArea(theme->getEditorBox());

	this->about = about;
	this->entryEditor = entryEditor;
	this->envEditor = envEditor;
	this->error = error;
	this->installer = installer;
	this->main = main;
	this->settings = settings;
	this->theme = theme;
	this->trash = trash;
}

Bootstrap_View::~Bootstrap_View() {
}
