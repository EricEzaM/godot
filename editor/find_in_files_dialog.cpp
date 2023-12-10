/*************************************************************************/
/*  find_in_files_dialog.cpp                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "editor/find_in_files_dialog.h"

#include "editor/editor_node.h"
#include "editor/editor_paths.h"
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"
#include "editor/find_in_files_searcher.h"
#include "editor_string_names.h"
#include "find_in_files_shared.h"
#include "find_in_files_tree.h"
#include "scene/gui/check_box.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/split_container.h"
#include "scene/gui/texture_rect.h"

void FindInFilesDialog2::_on_folder_selected(const String &p_path) {
	if (folder_line_edit->get_text() != p_path) {
		folder_line_edit->set_text(p_path);
		_run_search();
	}
}

void FindInFilesDialog2::_on_folder_text_changed(const String &p_string) {
	if (!p_string.begins_with("res://")) {
		WARN_PRINT("Search directory must be within project folder, res://");
		folder_line_edit->set_text(previous_folder_selection);
		folder_line_edit->set_caret_column(folder_line_edit->get_text().size());
		return;
	}
	_run_search();
}

void FindInFilesDialog2::_on_file_filter_toggled(bool p_toggled_on) {
	file_filter_line_edit->set_editable(p_toggled_on);
	_run_search();
}

void FindInFilesDialog2::_on_recent_file_filter_selected(int p_idx) {
	file_filter_chkbx->set_pressed(true);
	file_filter_line_edit->set_text(recent_file_filters_btn->get_popup()->get_item_text(p_idx));
	_run_search();
}

void FindInFilesDialog2::_on_match_regex_toggled(bool p_toggled_on) {
	if (p_toggled_on) {
		match_word_btn->set_pressed(false);
		match_word_btn->set_disabled(true);
	} else {
		match_word_btn->set_disabled(false);
	}

	_run_search();
}

void FindInFilesDialog2::_update_file_preview() {
	const TreeItem *selected = results->get_selected();
	if (!selected) {
		file_preview->clear_file();
		return;
	}

	const Array &ids = selected->get_meta("ids");
	if (ids.is_empty()) {
		file_preview->clear_file();
		return;
	}

	const FindInFilesSearcher::FindResult r = result_items[ids.front()];
	file_preview->open_file(r.path, r.start_line);
}

void FindInFilesDialog2::_update_selected_item() {
	TreeItem *selected = results->get_selected();
	if (!selected) {
		return;
	}

	const Array &ids = selected->get_meta("ids");
	if (ids.is_empty()) {
		return;
	}

	FindInFilesSearcher::FindResult r = result_items[ids.front()];
	if (!searcher->is_result_valid(r)) {
		selected->set_text(0, "INVALID");
		selected->set_custom_color(0, invalid_result_color);
	}
}

void FindInFilesDialog2::_on_open_file_requested(const String &p_path, int p_line, int p_column) {
	auto res = ScriptEditor::get_singleton()->open_file(p_path, true);
	ScriptEditor::get_singleton()->edit(res, p_line, p_column);
	hide();
}

void FindInFilesDialog2::_on_mode_changed() {
	if (mode == FIND_MODE) {
		set_title(TTR("Find in Files"));
		if (replace_dialog_btn) {
			remove_button(replace_dialog_btn);
		}
		if (replace_all_dialog_btn) {
			remove_button(replace_all_dialog_btn);
		}
		replace_hbc->hide();
		replace_preview->hide();
		replace_preview_label->hide();
	}
	if (mode == REPLACE_MODE) {
		set_title(TTR("Replace in Files"));
		replace_dialog_btn = add_button(TTR("Replace"), true, "replace");
		replace_all_dialog_btn = add_button(TTR("Replace All"), true, "replace_all");
		replace_hbc->show();
		replace_preview->show();
		replace_preview_label->show();
	}

	results->set_group_results_on_same_line(mode == FIND_MODE);
	results->remake_tree();
}

void FindInFilesDialog2::_run_search() {
	// Cancel current search.
	update_poll_timer->stop();
	searcher->stop();

	// Clear results
	results->reset();
	_update_file_preview();

	// Update searcher options for next search.
	searcher->set_case_sensitive(match_case_btn->is_pressed());
	searcher->set_whole_words(match_word_btn->is_pressed());
	searcher->set_use_regex(match_regex_btn->is_pressed());
	searcher->set_directory(folder_line_edit->get_text());
	if (file_filter_chkbx->is_pressed()) {
		searcher->set_file_filter(file_filter_line_edit->get_text());
	} else {
		searcher->set_file_filter("");
	}

	const String search_string = search_line_edit->get_text();
	searcher->set_search_text(search_string);

	if (search_string.is_empty()) {
		status_display->set_text(TTR("Type a search query to find in files."));
		return;
	}

	// Test if the search is valid and exit early if not.
	String message;
	if (searcher->is_valid(message)) {
		search_validation->hide();
	} else {
		search_validation->show();
		search_validation->set_tooltip_text(message);
		return;
	}

	// Start new search.
	searcher->start();
	update_poll_timer->start();
}

void FindInFilesDialog2::_update_search_status() {
	auto status = searcher->get_status();
	status_display->set_text(vformat("%s%s matches in %s%s files", status.results.size(), status.limit_reached ? "+" : "", status.files_with_matches, status.limit_reached ? "+" : ""));

	if (status.results.size() == 0) {
		_update_file_preview();
	}

	for (const FindInFilesSearcher::FindResult &r : status.results) {
		results->add_result(r);
		result_items[r.id] = r;
	}

	// Select first result when it is available - do not override user selection.
	results->select_first_non_root();

	if (status.finished) {
		update_poll_timer->stop();
	}
}

void FindInFilesDialog2::_save_recent_filters(bool p_save_to_editor_cfg) {
	// Filter not set, no need to save.
	if (!file_filter_chkbx->is_pressed()) {
		return;
	}

	// Save not needed if current filter is already at top of list.
	String current_filter = file_filter_line_edit->get_text();
	if (recent_filters.find(current_filter) == recent_filters.size()) {
		return;
	}

	// Push current to end if it exists.
	recent_filters.erase(current_filter);
	recent_filters.push_back(current_filter);

	// Save only the X most recent filters.
	const int count = MIN(15, recent_filters.size());
	const int start = MAX(0, recent_filters.size() - count);
	recent_filters = recent_filters.slice(start);

	_update_recent_filters_menu();

	if (p_save_to_editor_cfg) {
		Ref<ConfigFile> config;
		config.instantiate();
		config->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));

		const String section = "find_in_files";
		const String save_string = String::chr(0xFFFF).join(recent_filters);
		config->set_value(section, "recent_filters", save_string);

		config->save(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));
	}
}

void FindInFilesDialog2::_load_recent_filters() {
	Ref<ConfigFile> config;
	config.instantiate();
	config->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));

	const String section = "find_in_files";
	const String recent_filters_string = config->get_value("find_in_files", "recent_filters", "");
	Vector<String> recent_filters_from_config = recent_filters_string.split(String::chr(0xFFFF));

	recent_filters.clear();
	for (const String &filter : recent_filters_from_config) {
		if (filter.is_empty()) {
			continue;
		}
		recent_filters.push_back(filter);
	}
	_update_recent_filters_menu();
}

void FindInFilesDialog2::_update_recent_filters_menu() {
	PopupMenu *popup = recent_file_filters_btn->get_popup();
	popup->clear();
	if (recent_filters.is_empty()) {
		popup->add_item(TTR("No recent filters."), 0);
		popup->set_item_disabled(popup->get_item_index(0), true);
		return;
	}

	// Add in reverse as last item in the list is the newest, want newest on top.
	for (int i = recent_filters.size() - 1; i >= 0; --i) {
		popup->add_item(recent_filters[i]);
	}
}

void FindInFilesDialog2::_do_replace_on_selected() {
	// const TreeItem *selected = results->get_selected();
	// ERR_FAIL_COND_MSG(!selected, "Can't perform replace - nothing selected.");
	//
	// const String id = selected->get_meta("id");
	// ERR_FAIL_COND_MSG(id.is_empty(), "Can't perform replace - selected does not have ID metadata");
	//
	// FindInFilesSearcher::FindResult result = result_items[id];
	// if (searcher->replace_match(result, replace_line_edit->get_text())) {
	// 	_update_file_preview();
	// 	_update_selected_item();
	// 	ScriptEditor::get_singleton()->reload_scripts();
	// }
}

void FindInFilesDialog2::_do_replace_all() {
	// TODO
}

void FindInFilesDialog2::_update_replace_preview() {
	// const TreeItem *selected = results->get_selected();
	// ERR_FAIL_COND_MSG(!selected, "Can't update replace preview - nothing selected.");
	//
	// const Variant id = selected->get_meta("id");
	// ERR_FAIL_COND_MSG(!id.is_string(), "Can't update replace preview - selected does not have ID metadata");
	//
	// FindInFilesSearcher::FindResult result = result_items[id];
	// const String preview = searcher->get_replace_match_preview(result, replace_line_edit->get_text());
	//
	// replace_preview->set_text(preview);
}

void FindInFilesDialog2::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_load_recent_filters();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_save_recent_filters(true);
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible()) {
				search_line_edit->grab_focus();
				search_line_edit->select_all();
				if (!search_line_edit->get_text().is_empty()) {
					_run_search();
				}
			} else {
				_save_recent_filters(false);
			}
		} break;
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			invalid_result_color = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
			match_case_btn->set_icon(get_theme_icon(SNAME("MatchCase"), EditorStringName(EditorIcons)));
			match_word_btn->set_icon(get_theme_icon(SNAME("MatchWord"), EditorStringName(EditorIcons)));
			match_regex_btn->set_icon(get_theme_icon(SNAME("MatchRegex"), EditorStringName(EditorIcons)));

			status_display->add_theme_font_override("font", get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
			search_validation->set_texture(get_theme_icon("StatusError", EditorStringName(EditorIcons)));

			results->add_theme_font_override("font", EditorNode::get_singleton()->get_gui_base()->get_theme_font(SNAME("source"), EditorStringName(EditorFonts)));
			results->add_theme_font_size_override("font_size", EditorNode::get_singleton()->get_gui_base()->get_theme_font_size(SNAME("source_size"), EditorStringName(EditorFonts)));
		} break;
	}
}

void FindInFilesDialog2::custom_action(const String &p_string) {
	if (p_string == "replace") {
		_do_replace_on_selected();
	} else if (p_string == "replace_all") {
		// Replace all (including matches outside of search?)
		_do_replace_all();
	} else {
		ERR_FAIL_MSG(vformat("Bug: Custom Action '%s' is not handled!", p_string));
	}
}

void FindInFilesDialog2::ok_pressed() {
	FindInFilesPanel2::get_singleton()->add_search(searcher->create_input_data());
}

void FindInFilesDialog2::shortcut_input(const Ref<InputEvent> &p_event) {
	if (ED_IS_SHORTCUT("script_text_editor/find_in_files", p_event)) {
		set_find_in_files_mode(FIND_MODE);
		set_input_as_handled();
		return;
	}
	if (ED_IS_SHORTCUT("script_text_editor/replace_in_files", p_event)) {
		set_find_in_files_mode(REPLACE_MODE);
		set_input_as_handled();
		return;
	}

	AcceptDialog::shortcut_input(p_event);
}

FindInFilesDialog2::FindInFilesMode FindInFilesDialog2::get_dialog_mode() const {
	return mode;
}

void FindInFilesDialog2::set_find_in_files_mode(FindInFilesMode p_mode) {
	if (mode == p_mode) {
		return;
	}

	mode = p_mode;
	_on_mode_changed();
}

void FindInFilesDialog2::set_find_text(const String &p_text) {
	search_line_edit->set_text(p_text);
}

FindInFilesDialog2::FindInFilesDialog2() {
	searcher = memnew(FindInFilesSearcher);
	searcher->set_result_limit(100, false);

	set_ok_button_text("Open in Panel");
	set_process_shortcut_input(true);

	set_min_size(Size2(720 * EDSCALE, 600 * EDSCALE));
	set_title(TTR("Find in Files"));

	update_poll_timer = memnew(Timer);
	update_poll_timer->set_wait_time(0.05);
	update_poll_timer->connect(SNAME("timeout"), callable_mp(this, &FindInFilesDialog2::_update_search_status));
	add_child(update_poll_timer);

	VBoxContainer *main_vbc = memnew(VBoxContainer);
	main_vbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(main_vbc);

	// Search
	HBoxContainer *search_hbc = memnew(HBoxContainer);
	main_vbc->add_child(search_hbc);

	search_validation = memnew(TextureRect);
	search_validation->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	search_validation->hide();
	search_hbc->add_child(search_validation);

	search_line_edit = memnew(LineEdit);
	search_line_edit->set_clear_button_enabled(true);
	search_line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	search_line_edit->connect(SNAME("text_changed"), callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	search_hbc->add_child(search_line_edit);

	match_case_btn = memnew(Button);
	match_case_btn->set_flat(true);
	match_case_btn->set_toggle_mode(true);
	match_case_btn->set_tooltip_text(TTR("Match case"));
	match_case_btn->set_focus_mode(Control::FOCUS_CLICK);
	match_case_btn->connect(SNAME("toggled"), callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	search_hbc->add_child(match_case_btn);

	match_word_btn = memnew(Button);
	match_word_btn->set_flat(true);
	match_word_btn->set_toggle_mode(true);
	match_word_btn->set_tooltip_text(TTR("Match whole words (incompatible with Regex)"));
	match_word_btn->set_focus_mode(Control::FOCUS_CLICK);
	match_word_btn->connect(SNAME("toggled"), callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	search_hbc->add_child(match_word_btn);

	match_regex_btn = memnew(Button);
	match_regex_btn->set_flat(true);
	match_regex_btn->set_toggle_mode(true);
	match_regex_btn->set_tooltip_text(TTR("Use regular expressions (regex)"));
	match_regex_btn->set_focus_mode(Control::FOCUS_CLICK);
	match_regex_btn->connect(SNAME("toggled"), callable_mp(this, &FindInFilesDialog2::_on_match_regex_toggled));
	search_hbc->add_child(match_regex_btn);

	// Replace
	replace_hbc = memnew(HBoxContainer);
	main_vbc->add_child(replace_hbc);

	replace_line_edit = memnew(LineEdit);
	replace_line_edit->set_clear_button_enabled(true);
	replace_line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	replace_line_edit->connect("text_changed", callable_mp(this, &FindInFilesDialog2::_update_replace_preview).unbind(1));
	replace_hbc->add_child(replace_line_edit);
	replace_hbc->hide();

	// Directory, File Filter
	HBoxContainer *files_filter_hbc = memnew(HBoxContainer);
	main_vbc->add_child(files_filter_hbc);

	Label *dir_label = memnew(Label);
	dir_label->set_text("Folder");
	files_filter_hbc->add_child(dir_label);

	folder_line_edit = memnew(LineEdit);
	folder_line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	folder_line_edit->set_text(previous_folder_selection);
	folder_line_edit->connect(SNAME("text_changed"), callable_mp(this, &FindInFilesDialog2::_on_folder_text_changed));
	files_filter_hbc->add_child(folder_line_edit);

	folder_dialog = memnew(FileDialog);
	folder_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_DIR);
	folder_dialog->connect(SNAME("dir_selected"), callable_mp(this, &FindInFilesDialog2::_on_folder_selected));
	folder_dialog->set_title(TTR("Select a folder"));
	add_child(folder_dialog);

	Button *folder_btn = memnew(Button);
	folder_btn->set_text("...");
	folder_btn->connect(SNAME("pressed"), callable_mp(folder_dialog, &FileDialog::popup_file_dialog));
	files_filter_hbc->add_child(folder_btn);

	Label *file_filter_label = memnew(Label);
	file_filter_label->set_text("File Filter");
	files_filter_hbc->add_child(file_filter_label);

	file_filter_chkbx = memnew(CheckBox);
	file_filter_chkbx->set_pressed(true);
	file_filter_chkbx->connect(SNAME("toggled"), callable_mp(this, &FindInFilesDialog2::_on_file_filter_toggled));
	files_filter_hbc->add_child(file_filter_chkbx);

	file_filter_line_edit = memnew(LineEdit);
	file_filter_line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	file_filter_line_edit->set_text("*.gd,*.gdshader");
	file_filter_line_edit->set_editable(file_filter_chkbx->is_pressed());
	file_filter_line_edit->connect(SNAME("text_changed"), callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	files_filter_hbc->add_child(file_filter_line_edit);

	recent_file_filters_btn = memnew(MenuButton);
	recent_file_filters_btn->set_text("Recent Filters");
	recent_file_filters_btn->get_popup()->connect(SNAME("index_pressed"), callable_mp(this, &FindInFilesDialog2::_on_recent_file_filter_selected));
	files_filter_hbc->add_child(recent_file_filters_btn);

	// Status
	HBoxContainer *status_hbc = memnew(HBoxContainer);
	main_vbc->add_child(status_hbc);

	status_display = memnew(Label);
	status_display->set_text(TTR("Type a search query to find in files."));
	status_hbc->add_child(status_display);

	replace_preview_label = memnew(Label);
	replace_preview_label->set_text(TTR("After replacement:"));
	status_hbc->add_child(replace_preview_label);

	replace_preview = memnew(Label);
	status_hbc->add_child(replace_preview);

	// The results list & editor
	VSplitContainer *split = memnew(VSplitContainer);
	split->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbc->add_child(split);

	results = memnew(FindInFilesTree(true));
	results->set_stretch_ratio(0.75);
	results->set_group_results_on_same_line(true);
	results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesDialog2::_update_file_preview));
	results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesDialog2::_update_replace_preview));
	results->connect(SNAME("open_file_requested"), callable_mp(this, &FindInFilesDialog2::_on_open_file_requested));
	split->add_child(results);

	file_preview = memnew(FindInFilesFilePreview);
	split->add_child(file_preview);
}
