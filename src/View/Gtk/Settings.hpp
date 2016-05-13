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

#ifndef SETTING_DLG_GTK_INCLUDED
#define SETTING_DLG_GTK_INCLUDED
#include <gtkmm.h>
#include "../Settings.hpp"
#include <libintl.h>
#include <string>
#include "../ColorChooser.hpp"


class View_Gtk_Settings :
	public Gtk::Dialog,
	public View_Settings
{
	struct AdvancedSettingsTreeModel : public Gtk::TreeModelColumnRecord {
		Gtk::TreeModelColumn<bool> active;
		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> old_name;
		Gtk::TreeModelColumn<Glib::ustring> value;
		AdvancedSettingsTreeModel() {
			this->add(active);
			this->add(name);
			this->add(old_name);
			this->add(value);
		}
	};
	struct CustomOption_obj : public CustomOption {
		CustomOption_obj(std::string name, std::string old_name, std::string value, bool isActive) {
			this->name = name;
			this->old_name = old_name;
			this->value = value;
			this->isActive = isActive;
		}
	};
	private: AdvancedSettingsTreeModel asTreeModel;
	private: Glib::RefPtr<Gtk::ListStore> refAsListStore;
	private: bool event_lock = false;
	
	private: Gtk::Notebook tabbox;
	private: Gtk::ScrolledWindow scrAllEntries;
	private: Gtk::TreeView tvAllEntries;
	private: Gtk::VBox vbAllEntries;
	private: Gtk::HBox hbAllEntriesControl;
	private: Gtk::Button bttAddCustomEntry, bttRemoveCustomEntry;

	private: Gtk::VBox vbCommonSettings, vbAppearanceSettings;
	private: Gtk::Alignment alignCommonSettings;
	
	private: Pango::AttrList attrDefaultEntry;
	private: Pango::Attribute aDefaultEntry;
	//default entry group
	private: Gtk::Frame groupDefaultEntry;
	private: Gtk::Alignment alignDefaultEntry;
	private: Gtk::Label lblDefaultEntry;
	//Gtk::Table tblDefaultEntry;
	private: Gtk::RadioButton rbDefPredefined, rbDefSaved;
	private: Gtk::RadioButtonGroup rbgDefEntry;
	private: Gtk::VBox vbDefaultEntry;
	private: Gtk::HBox hbDefPredefined;
	//Gtk::SpinButton spDefPosition;
	private: Gtk::ComboBoxText cbDefEntry;
	private: Gtk::Entry txtDefEntry;
	private: std::map<int, std::string> defEntryValueMapping;
	private: Gtk::Button bttDefaultEntryHelp;
	private: Gtk::Image imgDefaultEntryHelp;

	//view group
	private: Gtk::Frame groupView;
	private: Gtk::Alignment alignView;
	private: Gtk::Label lblView;
	private: Gtk::VBox vbView;
	private: Gtk::CheckButton chkShowMenu, chkOsProber;
	private: Gtk::HBox hbTimeout;
	private: Gtk::CheckButton chkTimeout;
	private: Gtk::SpinButton spTimeout;
	private: Gtk::Label lblTimeout2;

	//kernel parameters
	private: Gtk::Frame groupKernelParams;
	private: Gtk::Alignment alignKernelParams;
	private: Gtk::Label lblKernelParams;
	private: Gtk::VBox vbKernelParams;
	private: Gtk::Entry txtKernelParams;
	private: Gtk::CheckButton chkGenerateRecovery;
	
	//screen resolution
	private: Gtk::Alignment alignResolutionAndTheme;
	private: Gtk::HBox hbResolutionAndTheme;
	private: Gtk::HBox hbResolution;
	private: Gtk::CheckButton chkResolution;
	private: Gtk::ComboBoxText cbResolution;

	public: View_Gtk_Settings() :
		bttAddCustomEntry(Gtk::Stock::ADD),
		bttRemoveCustomEntry(Gtk::Stock::REMOVE),
		rbDefPredefined(gettext("pre_defined: "), true),
		rbDefSaved(gettext("previously _booted entry"), true),
		lblDefaultEntry(gettext("default entry")),
		lblView(gettext("visibility")),
		chkShowMenu(gettext("show menu")),
		lblKernelParams(gettext("kernel parameters")),
		chkGenerateRecovery(gettext("generate recovery entries")),
		chkOsProber(gettext("look for other operating systems")),
		chkResolution(gettext("custom resolution: ")),
		cbResolution(true),
		imgDefaultEntryHelp(Gtk::Stock::HELP, Gtk::ICON_SIZE_BUTTON)
	{
		this->set_title("Grub Customizer - " + Glib::ustring(gettext("settings")));
		this->set_icon_name("grub-customizer");
		Gtk::Box* winBox = this->get_vbox();
		winBox->pack_start(tabbox);
		tabbox.append_page(alignCommonSettings, gettext("_General"), true);
		tabbox.append_page(vbAppearanceSettings, gettext("A_ppearance"), true);
		tabbox.append_page(vbAllEntries, gettext("_Advanced"), true);

		vbAllEntries.pack_start(hbAllEntriesControl, Gtk::PACK_SHRINK);
		vbAllEntries.pack_start(scrAllEntries);
		hbAllEntriesControl.add(bttAddCustomEntry);
		hbAllEntriesControl.add(bttRemoveCustomEntry);
		hbAllEntriesControl.set_border_width(5);
		hbAllEntriesControl.set_spacing(5);
		scrAllEntries.add(tvAllEntries);
		scrAllEntries.set_border_width(5);
		scrAllEntries.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		refAsListStore = Gtk::ListStore::create(asTreeModel);
		tvAllEntries.set_model(refAsListStore);
		tvAllEntries.append_column_editable(gettext("is active"), asTreeModel.active);
		tvAllEntries.append_column_editable(gettext("name"), asTreeModel.name);
		tvAllEntries.append_column_editable(gettext("value"), asTreeModel.value);
		refAsListStore->signal_row_changed().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_setting_row_changed));
		vbCommonSettings.set_spacing(15);
		vbAppearanceSettings.set_spacing(5);

		//default entry group
		vbCommonSettings.pack_start(groupDefaultEntry, Gtk::PACK_SHRINK);
		groupDefaultEntry.add(alignDefaultEntry);
		groupDefaultEntry.set_sensitive(false);
		groupDefaultEntry.set_label_widget(lblDefaultEntry);
		lblDefaultEntry.set_attributes(attrDefaultEntry);
		aDefaultEntry = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
		attrDefaultEntry.insert(aDefaultEntry);
		alignDefaultEntry.add(vbDefaultEntry);
		vbDefaultEntry.add(hbDefPredefined);
		vbDefaultEntry.add(rbDefSaved);

		bttDefaultEntryHelp.add(imgDefaultEntryHelp);

		hbDefPredefined.pack_start(rbDefPredefined, Gtk::PACK_SHRINK);
		hbDefPredefined.pack_start(cbDefEntry);
		hbDefPredefined.pack_start(txtDefEntry);
		hbDefPredefined.pack_start(bttDefaultEntryHelp, Gtk::PACK_SHRINK);

		hbDefPredefined.set_spacing(5);
		vbDefaultEntry.set_spacing(5);
		groupDefaultEntry.set_shadow_type(Gtk::SHADOW_NONE);
		alignDefaultEntry.set_padding(2, 2, 25, 2);
		rbDefPredefined.set_group(rbgDefEntry);
		rbDefSaved.set_group(rbgDefEntry);
		txtDefEntry.set_no_show_all(true);

		//set maximum size of the combobox
		Glib::ListHandle<Gtk::CellRenderer*> cellRenderers = cbDefEntry.get_cells();
		Gtk::CellRenderer* cellRenderer = *cellRenderers.begin();
		cellRenderer->set_fixed_size(200, -1);
		try {
			dynamic_cast<Gtk::CellRendererText&>(*cellRenderer).property_ellipsize().set_value(Pango::ELLIPSIZE_END);
		} catch (std::bad_cast const& e) {
			this->log("cannot set ellipsizing mode because of an cast error", Logger::ERROR);
		}

		//view group
		alignCommonSettings.add(vbCommonSettings);
		alignCommonSettings.set_padding(20, 0, 0, 0);
		vbCommonSettings.pack_start(groupView, Gtk::PACK_SHRINK);
		groupView.add(alignView);
		groupView.set_shadow_type(Gtk::SHADOW_NONE);
		groupView.set_label_widget(lblView);
		lblView.set_attributes(attrDefaultEntry);
		alignView.add(vbView);
		vbView.set_spacing(5);
		alignView.set_padding(2, 2, 25, 2);
		vbView.add(chkShowMenu);
		vbView.add(chkOsProber);
		vbView.add(hbTimeout);
		hbTimeout.set_spacing(5);

		Glib::ustring defaultEntryLabelText = gettext("Boot default entry after %1 Seconds");
		int timeoutInputPos = defaultEntryLabelText.find("%1");

		if (timeoutInputPos != -1) {
			chkTimeout.set_label(defaultEntryLabelText.substr(0, timeoutInputPos));
			lblTimeout2.set_text(defaultEntryLabelText.substr(timeoutInputPos + 2));
		} else {
			chkTimeout.set_label(defaultEntryLabelText);
		}

		hbTimeout.pack_start(chkTimeout, Gtk::PACK_SHRINK);
		hbTimeout.pack_start(spTimeout, Gtk::PACK_SHRINK);
		hbTimeout.pack_start(lblTimeout2, Gtk::PACK_SHRINK);

		spTimeout.set_digits(0);
		spTimeout.set_increments(1, 5);
		spTimeout.set_range(0, 1000000);

		//kernel parameters
		vbCommonSettings.pack_start(groupKernelParams, Gtk::PACK_SHRINK);
		groupKernelParams.add(alignKernelParams);
		groupKernelParams.set_shadow_type(Gtk::SHADOW_NONE);
		groupKernelParams.set_label_widget(lblKernelParams);
		lblKernelParams.set_attributes(attrDefaultEntry);
		alignKernelParams.add(vbKernelParams);
		alignKernelParams.set_padding(10, 2, 25, 2);
		vbKernelParams.add(txtKernelParams);
		vbKernelParams.add(chkGenerateRecovery);
		vbKernelParams.set_spacing(5);

		//screen resolution and theme chooser
		vbAppearanceSettings.pack_start(alignResolutionAndTheme, Gtk::PACK_SHRINK);
		alignResolutionAndTheme.add(hbResolutionAndTheme);
		alignResolutionAndTheme.set_padding(10, 0, 6, 0);
		hbResolutionAndTheme.set_homogeneous(true);
		hbResolutionAndTheme.set_spacing(15);

		//screen resolution
		hbResolutionAndTheme.pack_start(hbResolution);
		hbResolution.pack_start(chkResolution, Gtk::PACK_SHRINK);
		hbResolution.pack_start(cbResolution, Gtk::PACK_EXPAND_WIDGET);
		cbResolution.append("saved");


		//<signals>
		rbDefPredefined.signal_toggled().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_default_entry_predefined_toggeled));
		rbDefSaved.signal_toggled().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_default_entry_saved_toggeled));
		cbDefEntry.signal_changed().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_default_entry_changed));
		txtDefEntry.signal_changed().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_default_entry_changed));
		chkShowMenu.signal_toggled().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_showMenu_toggled));
		chkOsProber.signal_toggled().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_osProber_toggled));
		spTimeout.signal_value_changed().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_timeout_changed));
		chkTimeout.signal_toggled().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_timeout_checkbox_toggled));
		txtKernelParams.signal_changed().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_kernelparams_changed));
		chkGenerateRecovery.signal_toggled().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_recovery_toggled));
		chkResolution.signal_toggled().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_chkResolution_toggled));
		cbResolution.get_entry()->signal_changed().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_resolution_selected));
		bttAddCustomEntry.signal_clicked().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_add_row_button_clicked));
		bttRemoveCustomEntry.signal_clicked().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_remove_row_button_clicked));
		bttDefaultEntryHelp.signal_clicked().connect(sigc::mem_fun(this, &View_Gtk_Settings::signal_defEntryHelpClick));

		this->add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);
		this->set_default_size(500, 600);
	}

	public: Gtk::VBox& getCommonSettingsPane()
	{
		tabbox.remove(alignCommonSettings);
		alignCommonSettings.remove();
		return vbCommonSettings;
	}

	public: Gtk::VBox& getAppearanceSettingsPane()
	{
		tabbox.remove(vbAppearanceSettings);
		return vbAppearanceSettings;
	}

	public: void show()
	{
		this->show_all();
	}

	public: void hide()
	{
		Gtk::Dialog::hide();
	}

	public: void addEntryToDefaultEntryChooser(std::string const& value, std::string const& label)
	{
		event_lock = true;
		Glib::ustring mappedLabel = label;
		if (value == "0") {
			mappedLabel = gettext("(first entry)");
		} else if (value == "") {
			mappedLabel = gettext("(other…)");
		}

		this->defEntryValueMapping[this->defEntryValueMapping.size()] = value;
		cbDefEntry.append(mappedLabel);
		cbDefEntry.set_active(0);
		this->groupDefaultEntry.set_sensitive(true);
		event_lock = false;
	}

	public: void clearDefaultEntryChooser()
	{
		event_lock = true;
		cbDefEntry.remove_all();
		this->defEntryValueMapping.clear();
		this->groupDefaultEntry.set_sensitive(false); //if there's no entry to select, disable this area
		event_lock = false;
	}

	public: void clearResolutionChooser()
	{
		this->cbResolution.remove_all();
	}

	public: void addResolution(std::string const& resolution)
	{
		this->cbResolution.append(resolution);
	}

	public: std::string getSelectedDefaultGrubValue(){
		std::string value = this->defEntryValueMapping[cbDefEntry.get_active_row_number()];
		if (value != "") {
			return value;
		} else {
			return txtDefEntry.get_text();
		}
	}

	public: void addCustomOption(bool isActive, std::string const& name, std::string const& value)
	{
		this->event_lock = true;
		Gtk::TreeModel::iterator newItemIter = refAsListStore->append();
		(*newItemIter)[asTreeModel.active] = isActive;
		(*newItemIter)[asTreeModel.name] = name;
		(*newItemIter)[asTreeModel.old_name] = name;
		(*newItemIter)[asTreeModel.value] = value;
		this->event_lock = false;
	}

	public: void selectCustomOption(std::string const& name)
	{
		for (Gtk::TreeModel::iterator iter = refAsListStore->children().begin(); iter != refAsListStore->children().end(); iter++){
			if ((*iter)[asTreeModel.old_name] == name){
				tvAllEntries.set_cursor(refAsListStore->get_path(iter), *tvAllEntries.get_column(name == "" ? 1 : 2), name == "");
				break;
			}
		}
	}

	public: std::string getSelectedCustomOption()
	{
		std::vector<Gtk::TreeModel::Path> p = tvAllEntries.get_selection()->get_selected_rows();
		if (p.size() == 1)
			return (Glib::ustring)(*refAsListStore->get_iter(p.front()))[asTreeModel.name];
		else
			return "";
	}

	public: void removeAllSettingRows()
	{
		this->event_lock = true;
		this->refAsListStore->clear();
		this->event_lock = false;
	}

	public: CustomOption getCustomOption(std::string const& name)
	{
		for (auto& treeRow : this->refAsListStore->children()){
			if (treeRow[asTreeModel.old_name] == name)
				return CustomOption_obj(
					Glib::ustring(treeRow[asTreeModel.name]),
					Glib::ustring(treeRow[asTreeModel.old_name]),
					Glib::ustring(treeRow[asTreeModel.value]),
					treeRow[asTreeModel.active]
				 );
		}
		throw ItemNotFoundException("requested custom option not found", __FILE__, __LINE__);
	}

	public: void setActiveDefEntryOption(DefEntryType option)
	{
		this->event_lock = true;
		if (option == this->DEF_ENTRY_SAVED) {
			rbDefSaved.set_active(true);
			cbDefEntry.set_sensitive(false);
		}
		else if (option == this->DEF_ENTRY_PREDEFINED) {
			rbDefPredefined.set_active(true);
			cbDefEntry.set_sensitive(true);
		}
		this->event_lock = false;
	}

	public: DefEntryType getActiveDefEntryOption()
	{
		return rbDefSaved.get_active() ? DEF_ENTRY_SAVED : DEF_ENTRY_PREDEFINED;
	}

	public: void setDefEntry(std::string const& defEntry)
	{
		if (this->defEntryValueMapping.size() == 0) { // not initialized yet
			return;
		}

		this->event_lock = true;

		int pos = -1;
		bool isOtherEntry = defEntry == "";
		try {
			pos = this->getDefEntryPosition(defEntry);
		} catch (ItemNotFoundException const& e) {
			pos = this->getDefEntryPosition(""); // choose option "other"
			isOtherEntry = true;
		}

		if (!txtDefEntry.has_focus()) { // don't change ui while typing
			txtDefEntry.set_text(isOtherEntry ? defEntry : "");
			txtDefEntry.set_visible(isOtherEntry);
			cbDefEntry.set_active(pos);
		}

		this->event_lock = false;
	}

	public: void setShowMenuCheckboxState(bool isActive)
	{
		this->event_lock = true;
		chkShowMenu.set_active(isActive);
		chkTimeout.set_sensitive(isActive);
		if (!isActive) {
			chkTimeout.set_active(true);
		}
		this->event_lock = false;
	}

	public: bool getShowMenuCheckboxState()
	{
		return chkShowMenu.get_active();
	}

	public: void setOsProberCheckboxState(bool isActive)
	{
		this->event_lock = true;
		chkOsProber.set_active(isActive);
		this->event_lock = false;
	}

	public: bool getOsProberCheckboxState()
	{
		return chkOsProber.get_active();
	}

	public: void showHiddenMenuOsProberConflictMessage()
	{
		Gtk::MessageDialog(Glib::ustring::compose(gettext("This option doesn't work when the \"os-prober\" script finds other operating systems. Disable \"%1\" if you don't need to boot other operating systems."), chkOsProber.get_label())).run();
	}

	public: void setTimeoutValue(int value)
	{
		this->event_lock = true;
		spTimeout.set_value(value);
		this->event_lock = false;
	}

	public: void setTimeoutActive(bool active)
	{
		this->event_lock = true;
		chkTimeout.set_active(active);
		spTimeout.set_sensitive(active);
		this->event_lock = false;
	}

	public: int getTimeoutValue()
	{
		return spTimeout.get_value_as_int();
	}

	public: std::string getTimeoutValueString()
	{
		return Glib::ustring::format(this->getTimeoutValue());
	}

	public: bool getTimeoutActive()
	{
		return this->chkTimeout.get_active();
	}

	public: void setKernelParams(std::string const& params)
	{
		this->event_lock = true;
		txtKernelParams.set_text(params);
		this->event_lock = false;
	}

	public: std::string getKernelParams()
	{
		return txtKernelParams.get_text();
	}

	public: void setRecoveryCheckboxState(bool isActive)
	{
		this->event_lock = true;
		chkGenerateRecovery.set_active(isActive);
		this->event_lock = false;
	}

	public: bool getRecoveryCheckboxState()
	{
		return chkGenerateRecovery.get_active();
	}

	public: void setResolutionCheckboxState(bool isActive)
	{
		this->event_lock = true;
		chkResolution.set_active(isActive);
		cbResolution.set_sensitive(isActive);
		this->event_lock = false;
	}

	public: bool getResolutionCheckboxState()
	{
		return chkResolution.get_active();
	}

	public: void setResolution(std::string const& resolution)
	{
		this->event_lock = true;
		cbResolution.get_entry()->set_text(resolution);
		this->event_lock = false;
	}

	public: std::string getResolution()
	{
		return cbResolution.get_entry()->get_text();
	}

	public: void putThemeSelector(Gtk::Widget& themeSelector)
	{
		this->hbResolutionAndTheme.pack_start(themeSelector);
	}

	public: void putThemeEditArea(Gtk::Widget& themeEditArea)
	{
		this->vbAppearanceSettings.pack_start(themeEditArea);
	}

	private: int getDefEntryPosition(std::string const& name)
	{
		for (auto& entry : this->defEntryValueMapping) {
			if (entry.second == name) {
				return entry.first;
				break;
			}
		}

		throw ItemNotFoundException("default entry selection: unable to find position by name '" + name + "'", __FILE__, __LINE__);
	}


	private: void signal_setting_row_changed(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter)
	{
		if (!event_lock) {
			this->onCustomSettingChange((Glib::ustring)(*iter)[asTreeModel.old_name]);
		}
	}

	private: void signal_add_row_button_clicked()
	{
		if (!event_lock) {
			this->onAddCustomSettingClick();
		}
	}

	private: void signal_remove_row_button_clicked()
	{
		if (!event_lock) {
			this->onRemoveCustomSettingClick((Glib::ustring)(*tvAllEntries.get_selection()->get_selected())[asTreeModel.name]);
		}
	}

	private: void signal_default_entry_predefined_toggeled()
	{
		if (!event_lock){
			this->onDefaultSystemChange();
		}
	}

	private: void signal_default_entry_saved_toggeled()
	{
		if (!event_lock){
			this->onDefaultSystemChange();
		}
	}

	private: void signal_default_entry_changed()
	{
		if (!event_lock){
			this->onDefaultSystemChange();
		}
	}

	private: void signal_showMenu_toggled()
	{
		if (!event_lock){
			this->onShowMenuSettingChange();
		}
	}

	private: void signal_osProber_toggled()
	{
		if (!event_lock){
			this->onOsProberSettingChange();
		}
	}

	private: void signal_timeout_changed()
	{
		if (!event_lock){
			this->onTimeoutSettingChange();
		}
	}

	private: void signal_timeout_checkbox_toggled()
	{
		if (!event_lock){
			this->onTimeoutSettingChange();
		}
	}

	private: void signal_kernelparams_changed()
	{
		if (!event_lock){
			this->onKernelParamsChange();
		}
	}

	private: void signal_recovery_toggled()
	{
		if (!event_lock){
			this->onRecoverySettingChange();
		}
	}

	private: void signal_chkResolution_toggled()
	{
		if (!event_lock){
			this->onUseCustomResolutionChange();
		}
	}

	private: void signal_resolution_selected()
	{
		if (!event_lock){
			this->onCustomResolutionChange();
		}
	}

	private: void signal_defEntryHelpClick()
	{
		if (!event_lock){
			Gtk::MessageDialog helpDlg(
				gettext("This option allows choosing the operating system which should be selected when booting. "
				"By default this always is the first one. This option means that the default entry selection "
				"stays the same when placing other operating systems there.\n\nIn contrast to the first option the other "
				"choices are referenced by name. So the default system isn't changed when moving to another position.\n\n"
				"If you want to use another reference like \"always point to the second entry\" you can use the "
				"last option which allows typing a custom reference. Usage examples:\n"
				" 1 = \"always point to the second entry\" (counting starts by 0)\n"
				" test>0 = \"always point to the first entry of a submenu named test\""),
				true,
				Gtk::MESSAGE_INFO,
				Gtk::BUTTONS_OK,
				false
			);
			helpDlg.run();
		}
	}

	private: void on_response(int response_id)
	{
		this->onHide();
	}
};

#endif
