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

#ifndef ENTRY_ADD_DLG_INCLUDED
#define ENTRY_ADD_DLG_INCLUDED
#include "../Trash.hpp"

#include <gtkmm.h>
#include <libintl.h>
#include "../../lib/Type.hpp"
#include "Element/List.hpp"

class View_Gtk_Trash :
	public Gtk::Window,
	public View_Trash
{
	private: Gtk::ScrolledWindow scrEntryBox;
	private: View_Gtk_Element_List<Rule, Script> list;
	private: Gtk::Frame frmList;
	private: Gtk::VBox vbList;
	private: Gtk::HBox hbList;
	private: Gtk::Button bttRestore;
	private: Gtk::Button bttDelete;

	private: std::map<ViewOption, bool> options;

	private: Gtk::MenuItem miContext;
	private: Gtk::Menu contextMenu;
	private: Gtk::ImageMenuItem micRestore;
	private: Gtk::ImageMenuItem micDelete;

	private: bool event_lock = false;

	public: View_Gtk_Trash() :
		micRestore(Gtk::Stock::ADD),
		bttRestore(Gtk::Stock::UNDELETE),
		bttDelete(Gtk::Stock::DELETE),
		micDelete(Gtk::Stock::DELETE)
	{
		this->set_title(gettext("Add entry from trash"));
		this->set_icon_name("grub-customizer");
		this->set_default_size(650, 500);
		this->add(frmList);
		frmList.set_label(gettext("Removed items"));
		frmList.set_shadow_type(Gtk::SHADOW_NONE);
		frmList.add(vbList);
		vbList.pack_start(scrEntryBox);
		vbList.pack_start(hbList, Gtk::PACK_SHRINK);
		hbList.pack_start(bttRestore);
		hbList.pack_start(bttDelete);
		scrEntryBox.add(list);
		scrEntryBox.set_min_content_width(250);

		bttRestore.set_label(gettext("_Restore"));
		bttRestore.set_use_underline(true);
		bttRestore.set_tooltip_text(gettext("Restore selected entries"));
		bttRestore.set_border_width(5);
		bttRestore.set_sensitive(false);

		bttDelete.set_border_width(5);
		bttDelete.set_tooltip_text(gettext("permanently delete selected entries"));
		bttDelete.set_no_show_all(true);

		list.set_tooltip_column(1);

		list.ellipsizeMode = Pango::ELLIPSIZE_END;

		this->micDelete.set_sensitive(false);
		this->micDelete.set_tooltip_text(gettext("delete entries permanently - this action is available on custom entries only"));

		this->miContext.set_submenu(this->contextMenu);
		this->contextMenu.attach(this->micRestore, 0, 1, 0, 1);
		this->contextMenu.attach(this->micDelete, 0, 1, 1, 2);

		this->list.get_selection()->signal_changed().connect(sigc::mem_fun(this, &View_Gtk_Trash::signal_treeview_selection_changed));
		this->list.signal_row_activated().connect(sigc::mem_fun(this, &View_Gtk_Trash::signal_item_dblClick));
		this->list.signal_button_press_event().connect_notify(sigc::mem_fun(this, &View_Gtk_Trash::signal_button_press));
		this->list.signal_popup_menu().connect(sigc::mem_fun(this, &View_Gtk_Trash::signal_popup));
		this->micRestore.signal_activate().connect(sigc::mem_fun(this, &View_Gtk_Trash::restore_button_click));
		this->bttRestore.signal_clicked().connect(sigc::mem_fun(this, &View_Gtk_Trash::restore_button_click));
		this->micDelete.signal_activate().connect(sigc::mem_fun(this, &View_Gtk_Trash::delete_button_click));
		this->bttDelete.signal_clicked().connect(sigc::mem_fun(this, &View_Gtk_Trash::delete_button_click));
	}

	private: void signal_item_dblClick(Gtk::TreeModel::Path const& path, Gtk::TreeViewColumn* column)
	{
		this->list.get_selection()->unselect_all();
		this->list.get_selection()->select(path);
		this->onRestore();
		this->hide();
	}

	private: void restore_button_click()
	{
		this->onRestore();
	}

	private: void delete_button_click()
	{
		this->onDeleteClick();
	}

	public: void clear()
	{
		event_lock = true;
		list.refTreeStore->clear();
		event_lock = false;
	}

	public: std::list<Rule*> getSelectedEntries()
	{
		std::list<Rule*> result;
		std::vector<Gtk::TreePath> pathes = list.get_selection()->get_selected_rows();
		for (std::vector<Gtk::TreePath>::iterator pathIter = pathes.begin(); pathIter != pathes.end(); pathIter++) {
			Gtk::TreeModel::iterator elementIter = list.refTreeStore->get_iter(*pathIter);
			result.push_back((*elementIter)[list.treeModel.relatedRule]);
		}
		return result;
	}

	private: void addItem(View_Model_ListItem<Rule, Script> const& listItem)
	{
		this->list.addListItem(listItem, this->options, *this);
	}

	private: void setDeleteButtonEnabled(bool val)
	{
		this->bttDelete.set_visible(val);
	}

	public: void show()
	{
		this->show_all();
		this->miContext.show_all();
	}

	public: void hide()
	{
		this->Gtk::Window::hide();
	}

	public: void askForDeletion(std::list<std::string> const& names)
	{
		Glib::ustring question = gettext("This deletes the following entries:");
		question += "\n";
		for (std::list<std::string>::const_iterator iter = names.begin(); iter != names.end(); iter++) {
			question += *iter + "\n";
		}

		int response = Gtk::MessageDialog(question, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL).run();
		if (response == Gtk::RESPONSE_OK) {
			this->onDeleteClick();
		}
	}

	public: Gtk::Widget& getList()
	{
		this->remove();
		return this->frmList;
	}

	public: void setDeleteButtonVisibility(bool visibility)
	{
		bttDelete.set_visible(visibility);
		micDelete.set_sensitive(visibility);
	}

	public: void setOptions(std::map<ViewOption, bool> const& viewOptions)
	{
		this->options = viewOptions;
	}

	public: void selectEntries(std::list<Rule*> const& entries)
	{
		this->list.selectRules(entries);
	}

	public: void setRestoreButtonSensitivity(bool sensitivity)
	{
		this->bttRestore.set_sensitive(sensitivity);
	}

	private: void signal_treeview_selection_changed()
	{
		if (!event_lock) {
			this->onSelectionChange(this->list.getSelectedRules());
		}
	}

	private: void signal_button_press(GdkEventButton *event)
	{
		if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
			contextMenu.show_all();
			contextMenu.popup(event->button, event->time);
		}
	}

	private: bool signal_popup()
	{
		contextMenu.show_all();
		contextMenu.popup(0, gdk_event_get_time(nullptr));
		return true;
	}
};

#endif
