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

#ifndef GRUBENVEDITORGTK_H_
#define GRUBENVEDITORGTK_H_
#include "../EnvEditor.hpp"

#include <gtkmm.h>
#include <libintl.h>
#include "Element/PartitionChooser.hpp"

class View_Gtk_EnvEditor :
	public Gtk::Dialog,
	public View_EnvEditor
{
	private: Gtk::VBox vbContent;
	private: Gtk::Table tblLayout;
	private: Gtk::Label lblPartition;
	private: Gtk::Label lblType;
	private: Gtk::ComboBoxText cbType;
	private: Gtk::HSeparator separator;
	private: Gtk::ScrolledWindow scrSubmountpoints;
	private: Gtk::VBox vbSubmountpoints;
	private: Gtk::Label lblSubmountpoints;
	private: View_Gtk_Element_PartitionChooser* pChooser = nullptr;
	private: std::map<std::string, Gtk::Entry*> optionMap;
	private: std::map<std::string, Gtk::Label*> labelMap;
	private: std::map<std::string, Gtk::Image*> imageMap;
	private: std::map<std::string, Gtk::CheckButton*> subMountpoints;
	private: Gtk::CheckButton cbSaveConfig;
	private: Gtk::HButtonBox bbxSaveConfig;
	private: bool eventLock = true;

	private: std::string rootDeviceName;

	public: View_Gtk_EnvEditor() :
		lblPartition(gettext("_Partition:"), true),
		lblType(gettext("_Type:"), true),
		lblSubmountpoints(gettext("Submountpoints:")),
		cbSaveConfig(gettext("save this configuration"))
	{
		this->set_title("Grub Customizer environment setup");
		this->set_icon_name("grub-customizer");

		Gtk::Box& box = *this->get_vbox();
		box.add(this->vbContent);
		this->vbContent.add(this->tblLayout);
		this->vbContent.add(this->bbxSaveConfig);

		this->bbxSaveConfig.pack_start(this->cbSaveConfig);

		this->tblLayout.attach(this->lblPartition, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);

		this->tblLayout.attach(this->lblSubmountpoints, 0, 1, 1, 2);
		this->tblLayout.attach(this->scrSubmountpoints, 1, 2, 1, 2);

		this->tblLayout.attach(this->lblType, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
		this->tblLayout.attach(this->cbType, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK);

		this->tblLayout.attach(this->separator, 0, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK);

		this->scrSubmountpoints.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		this->scrSubmountpoints.add(this->vbSubmountpoints);
		this->scrSubmountpoints.set_size_request(-1, 50);

		this->lblSubmountpoints.set_no_show_all(true);
		this->scrSubmountpoints.set_no_show_all(true);

		this->cbType.append(gettext("Grub 2"));
		this->cbType.append(gettext("BURG"));
		this->cbType.set_active(0);
		this->cbType.signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EnvEditor::signal_bootloaderType_changed));

		lblPartition.set_alignment(Pango::ALIGN_RIGHT);
		lblType.set_alignment(Pango::ALIGN_RIGHT);
		lblSubmountpoints.set_alignment(Pango::ALIGN_RIGHT);

		this->tblLayout.set_spacings(10);
		this->tblLayout.set_border_width(10);

		this->add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		this->add_button(Gtk::Stock::APPLY, Gtk::RESPONSE_APPLY);

		this->signal_response().connect(sigc::mem_fun(this, &View_Gtk_EnvEditor::signal_response_action));

		this->eventLock = false;
	}

	public: ~View_Gtk_EnvEditor()
	{
		if (this->pChooser) {
			this->tblLayout.remove(*this->pChooser);
			delete this->pChooser;
			this->pChooser = nullptr;
		}
	}

	public: void setRootDeviceName(std::string const& rootDeviceName)
	{
		this->rootDeviceName = rootDeviceName;
	}

	public: void setEnvSettings(
		std::map<std::string, std::string> const& props,
		std::list<std::string> const& requiredProps,
		std::list<std::string> const& validProps
	)
	{
		this->eventLock = true;
		int pos = 4;

		for (auto& property : props) {
			Gtk::Label* label = nullptr;
			if (this->labelMap.find(property.first) == this->labelMap.end()) {
				label = Gtk::manage(new Gtk::Label(property.first + ":"));
				label->set_alignment(Pango::ALIGN_RIGHT);
				this->tblLayout.attach(*label, 0, 1, pos, pos+1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
				this->labelMap[property.first] = label;
			} else {
				label = this->labelMap[property.first];
			}

			Gtk::Entry* entry = nullptr;
			bool entryCreated = false;
			if (this->optionMap.find(property.first) == this->optionMap.end()) {
				entry = Gtk::manage(new Gtk::Entry());
				entryCreated = true;
				this->tblLayout.attach(*entry, 1, 2, pos, pos+1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK);
				label->set_mnemonic_widget(*entry);
				entry->signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EnvEditor::signal_optionModified));
				this->optionMap[property.first] = entry;
			} else {
				entry = this->optionMap[property.first];
			}

			if (entry->get_text() != property.second) {
				entry->set_text(property.second);
			}

			Gtk::Image* img = nullptr;
			if (this->imageMap.find(property.first) == this->imageMap.end()) {
				img = Gtk::manage(new Gtk::Image());
				this->tblLayout.attach(*img, 2, 3, pos, pos+1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
				this->imageMap[property.first] = img;
			} else {
				img = this->imageMap[property.first];
			}

			Glib::RefPtr<Gdk::Pixbuf> icon;
			if (std::find(validProps.begin(), validProps.end(), property.first) != validProps.end()) {
				icon = this->render_icon_pixbuf(Gtk::Stock::OK, Gtk::ICON_SIZE_BUTTON);
			} else if (std::find(requiredProps.begin(), requiredProps.end(), property.first) != requiredProps.end()) {
				icon = this->render_icon_pixbuf(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_BUTTON);
			} else {
				icon = this->render_icon_pixbuf(Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON);
			}

			img->set(icon);

			if (this->get_visible()) {
				this->tblLayout.show_all();
			}
			pos++;
		}
		this->eventLock = false;
	}

	public: std::map<std::string, std::string> getEnvSettings()
	{
		std::map<std::string, std::string> result;
		for (auto& option : this->optionMap) {
			result[option.first] = option.second->get_text();
		}
		return result;
	}

	public: int getBootloaderType() const
	{
		return this->cbType.get_active_row_number();
	}

	public: void show(bool resetPartitionChooser = false)
	{
		this->eventLock = true;
		if (this->pChooser != nullptr) {
			this->tblLayout.remove(*pChooser);
		}

		if (!this->pChooser) {
			this->pChooser = new View_Gtk_Element_PartitionChooser("", *this->deviceDataList, true, this->rootDeviceName);
			this->pChooser->signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EnvEditor::signal_partitionChanged));
		}
		if (resetPartitionChooser) {
			this->pChooser->set_active(0);
		}

		this->tblLayout.attach(*pChooser, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK);
		this->lblPartition.set_mnemonic_widget(*pChooser);

		this->show_all();

		this->eventLock = false;
	}

	public: void hide()
	{
		Gtk::Dialog::hide();
	}

	public: void removeAllSubmountpoints()
	{
		for (auto& subMountpoint : this->subMountpoints) {
			this->vbSubmountpoints.remove(*subMountpoint.second);
			delete subMountpoint.second;
		}
		this->subMountpoints.clear();

		this->scrSubmountpoints.hide();
		this->lblSubmountpoints.hide();
	}

	public: void addSubmountpoint(std::string const& name, bool isActive)
	{
		Gtk::CheckButton* cb = new Gtk::CheckButton(name);
		cb->set_active(isActive);
		cb->signal_toggled().connect(sigc::bind<Gtk::CheckButton&>(sigc::mem_fun(this, &View_Gtk_EnvEditor::signal_submountpointToggled), *cb));
		this->vbSubmountpoints.pack_start(*cb, Gtk::PACK_SHRINK);
		this->subMountpoints[name] = cb;

		this->scrSubmountpoints.show();
		vbSubmountpoints.show_all();
		this->lblSubmountpoints.show();
	}

	public: void setSubmountpointSelectionState(std::string const& submountpoint, bool new_isSelected)
	{
		this->subMountpoints[submountpoint]->set_active(new_isSelected);
	}

	public: void showErrorMessage(MountExceptionType type)
	{
		switch (type){
			case MOUNT_FAILED:       Gtk::MessageDialog(gettext("Mount failed!")).run(); break;
			case UMOUNT_FAILED:      Gtk::MessageDialog(gettext("umount failed!")).run(); break;
			case MOUNT_ERR_NO_FSTAB: Gtk::MessageDialog(gettext("This seems not to be a root file system (no fstab found)")).run(); break;
			case SUB_MOUNT_FAILED:   Gtk::MessageDialog(gettext("Couldn't mount the selected partition")).run(); break;
			case SUB_UMOUNT_FAILED:  Gtk::MessageDialog(gettext("Couldn't umount the selected partition")).run(); break;
		}
	}

	public: Gtk::Widget& getContentBox()
	{
		this->get_vbox()->remove(this->vbContent);
		return this->vbContent;
	}

	private: void signal_partitionChanged()
	{
		if (!this->eventLock) {
			std::string selectedUuid = this->pChooser->getSelectedUuid();
			if (selectedUuid != "") {
				selectedUuid = "UUID=" + selectedUuid;
			}
			this->onSwitchPartition(selectedUuid);
		}
	}

	private: void signal_bootloaderType_changed()
	{
		if (!this->eventLock) {
			this->onSwitchBootloaderType(this->cbType.get_active_row_number());
		}
	}

	private: void signal_optionModified()
	{
		if (!this->eventLock) {
			this->onOptionChange();
		}
	}

	private: void signal_response_action(int response_id)
	{
		if (response_id == Gtk::RESPONSE_CLOSE || response_id == Gtk::RESPONSE_DELETE_EVENT) {
			this->onExitClick();
		} else if (response_id == Gtk::RESPONSE_APPLY) {
			this->onApplyClick(this->cbSaveConfig.get_active());
		}
	}

	private: void signal_submountpointToggled(Gtk::CheckButton& sender)
	{
		if (!eventLock) {
			if (sender.get_active()) {
				this->onMountSubmountpointClick(sender.get_label());
			} else {
				this->onUmountSubmountpointClick(sender.get_label());
			}
		}
	}
};

#endif /* GRUBENVEDITOR_H_ */
