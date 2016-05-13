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

#ifndef ENTRYEDITDLGGTK_H_
#define ENTRYEDITDLGGTK_H_
#include "../EntryEditor.hpp"

#include "../../lib/Helper.hpp"
#include <libintl.h>
#include <gtkmm.h>

#include "Element/PartitionChooser.hpp"

class View_Gtk_EntryEditor :
	public View_EntryEditor,
	public Gtk::Dialog
{
	private: Gtk::VBox vbMainSections;
	private: Gtk::TextView tvSource;
	private: Gtk::VBox vbSource;
	private: Gtk::ScrolledWindow scrSource;
	private: Gtk::VBox vbSourceError;
	private: Gtk::Label lblSourceError;
	private: Gtk::Image imgSourceError;
	private: Gtk::Frame frmSource;
	private: Gtk::Table tblOptions;
	private: std::map<std::string, Gtk::Widget*> optionMap;
	private: std::map<std::string, Gtk::Widget*> optionContainerMap;
	private: std::map<std::string, Gtk::Label*> labelMap;
	private: std::map<std::string, Gtk::Image*> iconMap;
	private: Gtk::ComboBoxText cbType;
	private: Gtk::Image imgType;
	private: Gtk::Label lblType;
	private: Gtk::Entry txtName;
	private: Gtk::Label lblName;
	private: Gtk::Image imgName;
	private: Gtk::Button* bttApply = nullptr;
	private: bool lock_state = false;

	private: Rule* rulePtr = nullptr;

	public:	View_Gtk_EntryEditor() :
		lblType(gettext("_Type:"), true),
		frmSource(gettext("Boot sequence")),
		lblName(gettext("_Name:"), true),
		lblSourceError(gettext("Error building boot sequence.\nCheck parameters!")),
		imgSourceError(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG)
	{
		this->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		this->bttApply = this->add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

		this->set_default_size(500, 600);
		this->set_title(Glib::ustring() + gettext("Entry editor") + " - Grub Customizer");
		this->set_icon_name("grub-customizer");

		Gtk::Box& vbMain = *this->get_vbox();
		vbMain.pack_start(this->vbMainSections);

		vbMainSections.pack_start(this->tblOptions, Gtk::PACK_SHRINK);
		vbMainSections.pack_start(this->frmSource);

		frmSource.add(vbSource);
		frmSource.set_shadow_type(Gtk::SHADOW_NONE);
		frmSource.set_border_width(5);
		frmSource.set_margin_top(10);
		frmSource.set_no_show_all(true);

		vbSource.pack_start(vbSourceError, Gtk::PACK_EXPAND_PADDING);
		vbSource.pack_start(scrSource, Gtk::PACK_EXPAND_WIDGET);

		vbSourceError.pack_start(imgSourceError, Gtk::PACK_SHRINK);
		vbSourceError.pack_start(lblSourceError, Gtk::PACK_SHRINK);

		lblSourceError.set_justify(Gtk::JUSTIFY_CENTER);

		lblSourceError.set_no_show_all(true);
		imgSourceError.set_no_show_all(true);
		vbSourceError.set_no_show_all(true);

		scrSource.add(this->tvSource);
		scrSource.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		scrSource.set_shadow_type(Gtk::SHADOW_IN);
		scrSource.set_no_show_all(true);

		tblOptions.attach(this->lblName, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK, 5, 5);
		tblOptions.attach(this->txtName, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 5, 5);
		tblOptions.attach(this->imgName, 2, 3, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK, 5, 5);
		lblName.set_mnemonic_widget(txtName);
		lblName.set_alignment(Pango::ALIGN_RIGHT);
		lblName.set_no_show_all(true);
		txtName.set_no_show_all(true);
		imgName.set_no_show_all(true);

		tblOptions.attach(this->lblType, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK, 5, 5);
		tblOptions.attach(this->cbType, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 5, 5);
		tblOptions.attach(this->imgType, 2, 3, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK, 5, 5);
		lblType.set_mnemonic_widget(cbType);
		lblType.set_alignment(Pango::ALIGN_RIGHT);

		this->tvSource.set_accepts_tab(false);

		this->signal_response().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_response_action));
		this->txtName.signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_nameModified));
		this->cbType.signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_nameModified));
		this->cbType.signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_typeModified));

		this->tvSource.signal_key_release_event().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_sourceModified));
	}

	public:	void setSourcecode(std::string const& source)
	{
		this->vbSourceError.hide();
		this->lblSourceError.hide();
		this->imgSourceError.hide();
		this->scrSource.show();
		this->tvSource.show();
		this->frmSource.set_shadow_type(Gtk::SHADOW_NONE);

		std::string optimizedSource = Helper::str_replace("\n\t", "\n", source);
		if (optimizedSource[0] == '\t') {
			optimizedSource = optimizedSource.substr(1);
		}
		this->tvSource.get_buffer()->set_text(optimizedSource);
	}

	public:	std::string getSourcecode()
	{
		std::string optimizedSourcecode = this->tvSource.get_buffer()->get_text();
		std::string withIndent = Helper::str_replace("\n", "\n\t", optimizedSourcecode);
		if (withIndent.size() >= 2 && withIndent.substr(withIndent.size() - 2) == "\n\t") {
			withIndent.replace(withIndent.size() - 2, 2, "\n");
		} else if (withIndent.size() >= 1 && withIndent[withIndent.size() - 1] != '\n') {
			withIndent += '\n'; // add trailing slash
		}
		return "\t" + withIndent;
	}

	public:	void showSourceBuildError()
	{
		this->vbSourceError.show();
		this->lblSourceError.show();
		this->imgSourceError.show();
		this->scrSource.hide();
		this->tvSource.hide();
		this->frmSource.set_shadow_type(Gtk::SHADOW_ETCHED_IN); // looks better when there's no bordered text field
	}

	public:	void setApplyEnabled(bool value)
	{
		this->bttApply->set_sensitive(value);
	}

	public:	void addOption(std::string const& name, std::string const& value)
	{
		std::map<std::string, int> fixedElements;
		fixedElements["partition_uuid"] = 2;
		fixedElements["iso_path_full"] = 3;
		fixedElements["memtest_image_full"] = 4;

		int pos = fixedElements.find(name) != fixedElements.end() ? fixedElements[name] : this->optionMap.size() + fixedElements.size() + 2; // +2 because of type selection and name input
		Gtk::Label* label = Gtk::manage(new Gtk::Label(this->mapOptionName(name) + ":", true));
		label->set_alignment(Pango::ALIGN_RIGHT);
		this->tblOptions.attach(*label, 0, 1, pos, pos+1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK, 5, 5);
		this->labelMap[name] = label;

		Gtk::Widget* groupWidget = NULL;
		Gtk::Widget* formWidget = NULL;
		bool showValidator = true;
		if (name == "partition_uuid" && this->deviceDataList != NULL) {
			View_Gtk_Element_PartitionChooser* pChooserDD = Gtk::manage(new View_Gtk_Element_PartitionChooser(value, *this->deviceDataList));
			pChooserDD->signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_optionsModified));
			formWidget = groupWidget = pChooserDD;
		} else if (name == "iso_path_full" || name == "memtest_image_full") {
			Gtk::FileChooserButton* isoChooser = Gtk::manage(new Gtk::FileChooserButton());
			if (value == "") {
				isoChooser->unselect_all();
			} else {
				isoChooser->set_filename(value);
			}

			isoChooser->signal_file_set().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_optionsModified));
			formWidget = groupWidget = isoChooser;
		} else if (name == "iso_path" || name == "memtest_image") {
			Gtk::HBox* hbFieldGroup = Gtk::manage(new Gtk::HBox());
			Gtk::Entry* entry = Gtk::manage(new Gtk::Entry());
			entry->set_text(value);
			entry->signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_optionsModified));
			hbFieldGroup->pack_start(*entry, Gtk::PACK_EXPAND_WIDGET);
			Gtk::Button* bttFileChooser = Gtk::manage(new Gtk::Button(gettext("Choose…")));
			std::list<std::string> oldProps;
			oldProps.push_back(name);
			oldProps.push_back("partition_uuid");
			bttFileChooser->signal_clicked().connect(sigc::bind<std::string>(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_fileChooseButtonClicked), name + "_full", oldProps));
			hbFieldGroup->pack_start(*bttFileChooser, Gtk::PACK_SHRINK);
			formWidget = entry;
			groupWidget = hbFieldGroup;
		} else {
			Gtk::Entry* entry = Gtk::manage(new Gtk::Entry());
			entry->set_text(value);
			entry->signal_changed().connect(sigc::mem_fun(this, &View_Gtk_EntryEditor::signal_optionsModified));
			formWidget = groupWidget = entry;
		}

		this->tblOptions.attach(*groupWidget, 1, 2, pos, pos+1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK, 5, 5);
		label->set_mnemonic_widget(*formWidget);

		this->optionMap[name] = formWidget;
		this->optionContainerMap[name] = groupWidget;

		if (showValidator) {
			Gtk::Image* imgValidator = Gtk::manage(new Gtk::Image());
			this->tblOptions.attach(*imgValidator, 2, 3, pos, pos+1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK, 5, 5);
			this->iconMap[name] = imgValidator;
		}

		if (this->get_visible()) {
			this->tblOptions.show_all();
		}
	}

	public:	void setOptions(std::map<std::string, std::string> options)
	{
		this->removeOptions();
		for (std::map<std::string, std::string>::iterator iter = options.begin(); iter != options.end(); iter++) {
			this->addOption(iter->first, iter->second);
		}
	}

	public:	std::map<std::string, std::string> getOptions() const
	{
		std::map<std::string, std::string> result;
		for (std::map<std::string, Gtk::Widget*>::const_iterator iter = this->optionMap.begin(); iter != this->optionMap.end(); iter++) {
			try {
				result[iter->first] = dynamic_cast<View_Gtk_Element_PartitionChooser&>(*iter->second).getSelectedUuid();
			} catch (std::bad_cast const& e) {
				try {
					result[iter->first] = dynamic_cast<Gtk::Entry&>(*iter->second).get_text();
				} catch (std::bad_cast const& e) {
					try {
						result[iter->first] = dynamic_cast<Gtk::FileChooserButton&>(*iter->second).get_filename();
					} catch (std::bad_cast const& e) {
						throw BadCastException("unable to cast element '" + iter->first + "'", __FILE__, __LINE__);
					}
				}
			}
		}
		return result;
	}

	public:	void removeOptions()
	{
		for (std::map<std::string, Gtk::Label*>::iterator iter = this->labelMap.begin(); iter != this->labelMap.end(); iter++) {
			this->tblOptions.remove(*iter->second);
		}
		this->labelMap.clear();
		for (std::map<std::string, Gtk::Widget*>::iterator iter = this->optionContainerMap.begin(); iter != this->optionContainerMap.end(); iter++) {
			this->tblOptions.remove(*iter->second);
		}
		this->optionContainerMap.clear();
		this->optionMap.clear();

		for (std::map<std::string, Gtk::Image*>::iterator iter = this->iconMap.begin(); iter != this->iconMap.end(); iter++) {
			this->tblOptions.remove(*iter->second);
		}
		this->iconMap.clear();
	}

	public:	void setRulePtr(Rule* rulePtr)
	{
		this->rulePtr = rulePtr;
	}

	public:	Rule* getRulePtr()
	{
		return this->rulePtr;
	}

	public:	void show()
	{
		Gtk::Window::show_all();

		if (this->txtName.get_visible()) {
			this->set_focus(this->txtName);
		} else {
			this->set_focus(this->tvSource);
		}
	}

	public:	void hide()
	{
		Gtk::Window::hide();
	}

	public:	void setAvailableEntryTypes(std::list<std::string> const& names)
	{
		this->cbType.remove_all();
		for (std::list<std::string>::const_iterator iter = names.begin(); iter != names.end(); iter++) {
			this->cbType.append(*iter);
		}
		this->cbType.append(gettext("Other"));
		this->cbType.append(gettext("(script code)"));
	}

	public:	void selectType(std::string const& name)
	{
		std::string name2 = name;
		if (name2 == "") {
			name2 = gettext("Other");
		} else if (name2 == "[TEXT]") {
			name2 = gettext("(script code)");
		}

		this->lock_state = true;
		if (name == "[NONE]") {
			this->cbType.unset_active();
			this->frmSource.hide();
		} else {
			this->cbType.set_active_text(name2);
			this->frmSource.show();
			this->frmSource.show_all_children(true);
		}
		this->lock_state = false;
	}

	public:	std::string getSelectedType() const
	{
		if (this->cbType.get_active_text() == gettext("Other")) {
			return "";
		} else if (this->cbType.get_active_text() == gettext("(script code)")) {
			return "[TEXT]";
		} else {
			return this->cbType.get_active_text();
		}
	}

	public:	void setName(std::string const& name)
	{
		this->txtName.set_text(name);
	}

	public:	std::string getName()
	{
		return this->txtName.get_text();
	}

	public:	void setNameFieldVisibility(bool visible)
	{
		this->txtName.set_visible(visible);
		this->lblName.set_visible(visible);
		this->imgName.set_visible(visible);
	}

	public:	void setErrors(std::list<std::string> const& errors)
	{
		for (auto& validateIcon : this->iconMap) {
			bool isValid = std::find(errors.begin(), errors.end(), validateIcon.first) == errors.end();
			validateIcon.second->set(this->render_icon_pixbuf(isValid ? Gtk::Stock::OK : Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_BUTTON));
		}
	}

	public:	void setNameIsValid(bool valid)
	{
		this->imgName.set(this->render_icon_pixbuf(valid ? Gtk::Stock::OK : Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_BUTTON));
	}

	public:	void setTypeIsValid(bool valid)
	{
		this->imgType.set(this->render_icon_pixbuf(valid ? Gtk::Stock::OK : Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_BUTTON));
	}

	private: void signal_response_action(int response_id)
	{
		if (response_id == Gtk::RESPONSE_OK){
			this->onApplyClick();
		}
		this->hide();
	}

	private: bool signal_sourceModified(GdkEventKey* event)
	{
		if (!this->lock_state) {
			this->onSourceModification();
		}
		return true;
	}

	private: void signal_optionsModified() {
		if (!this->lock_state) {
			this->onOptionModification();
		}
	}

	private: void signal_nameModified()
	{
		if (!this->lock_state) {
			this->onNameChange();
		}
	}

	private: void signal_typeModified()
	{
		if (!this->lock_state) {
			this->onTypeSwitch(this->getSelectedType());
		}
	}

	private: void signal_fileChooseButtonClicked(std::string newProperty, std::list<std::string> oldProperties)
	{
		Gtk::FileChooserDialog dlg(gettext("Choose file…"), Gtk::FILE_CHOOSER_ACTION_OPEN);
		dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dlg.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_APPLY);
		int response = dlg.run();
		if (response == Gtk::RESPONSE_APPLY) {
			this->onFileChooserSelection(newProperty, dlg.get_filename(), oldProperties);
		}
	}

	private: virtual std::string mapOptionName(std::string const& name)
	{
		if (name == "partition_uuid")
			return gettext("_Partition");
		else if (name == "initramfs")
			return gettext("_Initial ramdisk");
		else if (name == "linux_image")
			return gettext("_Linux image");
		else if (name == "memtest_image")
			return gettext("_Memtest image");
		else if (name == "iso_path")
			return gettext("Path to iso file");
		else if (name == "other_params")
			return gettext("Kernel params");
		else if (name == "iso_path_full")
			return gettext("I_SO image");
		else if (name == "memtest_image_full")
			return gettext("_Memtest-Image");
		else
			return Helper::str_replace("_", "__", name);
	}
};

#endif /* ENTRYEDITDLGGTK_H_ */
