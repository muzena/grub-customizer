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

#ifndef GRUB_INSTALL_DLG_GTK_INCLUDED
#define GRUB_INSTALL_DLG_GTK_INCLUDED
#include "../Installer.hpp"

#include <gtkmm.h>
#include <libintl.h>

class View_Gtk_Installer :
	public Gtk::Dialog,
	public View_Installer
{
	private: Gtk::Label lblDescription;
	private: Gtk::HBox hbDevice;
	private: Gtk::Label lblDevice, lblInstallInfo;
	private: Gtk::Entry txtDevice;

	public:	View_Gtk_Installer() :
		lblDescription(gettext("Install the bootloader to MBR and put some\nfiles to the bootloaders data directory\n(if they don't already exist)."), Pango::ALIGN_LEFT),
		lblDevice(gettext("_Device: "), Pango::ALIGN_LEFT, Pango::ALIGN_CENTER, true)
	{
		Gtk::Box* vbDialog = this->get_vbox();
		this->set_icon_name("grub-customizer");
		vbDialog->pack_start(lblDescription, Gtk::PACK_SHRINK);
		vbDialog->pack_start(hbDevice);
		vbDialog->pack_start(lblInstallInfo);
		hbDevice.pack_start(lblDevice, Gtk::PACK_SHRINK);
		hbDevice.pack_start(txtDevice);
		txtDevice.set_text("/dev/sda");
		this->set_title(gettext("Install to MBR"));
		vbDialog->set_spacing(5);
		lblDevice.set_mnemonic_widget(txtDevice);
		this->set_border_width(5);
		this->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		this->add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
		this->set_default_response(Gtk::RESPONSE_OK);
		txtDevice.set_activates_default(true);

		this->signal_response().connect(sigc::mem_fun(this, &View_Gtk_Installer::signal_grub_install_dialog_response));
	}

	public:	void show()
	{
		this->show_all();
	}

	public:	void showMessageGrubInstallCompleted(std::string const& msg)
	{
		std::string output = msg;
		if (output == ""){
			Gtk::MessageDialog msg(gettext("The bootloader has been installed successfully"));
			msg.run();
			this->hide();
		} else {
			Gtk::MessageDialog msg(gettext("Error while installing the bootloader"), false, Gtk::MESSAGE_ERROR);
			msg.set_secondary_text(output);
			msg.run();
		}
		this->set_response_sensitive(Gtk::RESPONSE_OK, true);
		this->set_response_sensitive(Gtk::RESPONSE_CANCEL, true);
		txtDevice.set_sensitive(true);
		lblInstallInfo.set_text("");
	}

	public:	private: void signal_grub_install_dialog_response(int response_id)
	{
		if (response_id == Gtk::RESPONSE_OK){
			if (txtDevice.get_text().length()){
				this->set_response_sensitive(Gtk::RESPONSE_OK, false);
				this->set_response_sensitive(Gtk::RESPONSE_CANCEL, false);
				txtDevice.set_sensitive(false);
				lblInstallInfo.set_text(gettext("installing the bootloader…"));

				this->onInstallClick(txtDevice.get_text());
			} else {
				Gtk::MessageDialog(gettext("Please type a device string!")).run();
			}
		} else {
			this->hide();
		}
	}
};
#endif
