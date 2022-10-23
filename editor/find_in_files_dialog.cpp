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

#include "find_in_files_dialog.h"

#include "editor_node.h"
#include "editor_paths.h"
#include "editor_scale.h"
#include "editor_settings.h"
#include "find_in_files_searcher.h"
#include "scene/gui/file_dialog.h"

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

void FindInFilesDialog2::_on_match_regex_toggled(bool p_toggled) {
	// Match word is not compatible with regex.
	if (p_toggled) {
		match_word_btn->set_pressed(false);
		match_word_btn->set_disabled(true);
	} else {
		match_word_btn->set_disabled(false);
	}

	_run_search();
}

void FindInFilesDialog2::_set_editor(ScriptEditorBase *p_editor) {
	if (editor_container->get_child_count(false) == 1) {
		Node *node = editor_container->get_child(0, false);
		editor_container->remove_child(node);
		node->queue_delete();
	}

	editor = p_editor;
	if (editor) {
		editor_container->add_child(editor);
		// Read only editing for now. Live editing in the dialog is in the 'too hard' basket.
		Control *base_editor = editor->get_base_editor();
		if (base_editor->has_method("set_editable")) {
			base_editor->call("set_editable", false);
		}
	}
}

void FindInFilesDialog2::_update_mini_editor() {
	TreeItem *selected = results->get_selected();
	if (!selected) {
		return;
	}

	String id = selected->get_meta("id");
	if (id.is_empty()) {
		return;
	}

	FindInFilesSearcher::FindResult r = result_items[id];
	current_file_display->set_text(r.path.get_file());
	current_file_folder_display->set_text(r.path.replace(r.path.get_file(), ""));

	Ref<Resource> file = ScriptEditor::get_singleton()->open_file(r.path, false);
	if (file.is_valid()) {
		_set_editor(ScriptEditor::get_singleton()->create_script_editor(file));
		if (editor) {
			editor->set_edited_resource(file);
			editor->enable_editor();
			editor->goto_line_centered(r.start_line);
		}
	}
}

void FindInFilesDialog2::_update_selected_item() {
	TreeItem *selected = results->get_selected();
	if (!selected) {
		return;
	}

	String id = selected->get_meta("id");
	if (id.is_empty()) {
		return;
	}

	FindInFilesSearcher::FindResult r = result_items[id];
	if (!searcher->is_result_valid(r)) {
		selected->set_text(0, "INVALID");
		selected->set_custom_color(0, invalid_result_color);
	}
}

void FindInFilesDialog2::_on_result_activated() {
	TreeItem *selected = results->get_selected();
	if (!selected) {
		return;
	}

	String path = selected->get_metadata(0);
	if (path.is_empty()) {
		return;
	}

	ScriptEditor::get_singleton()->open_file(path, true);
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
	}
	if (mode == REPLACE_MODE) {
		set_title(TTR("Replace in Files"));
		replace_dialog_btn = add_button(TTR("Replace"), true, "replace");
		replace_all_dialog_btn = add_button(TTR("Replace All"), "replace_all");
		replace_hbc->show();
		replace_preview->show();
	}
}

void FindInFilesDialog2::_run_search() {
	// Cancel current search.
	update_poll_timer->stop();
	searcher->stop();

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

	// Test if the search is valid and exit early if not.
	String message;
	if (searcher->is_valid(message)) {
		search_validation->hide();
	} else {
		search_validation->show();
		search_validation->set_tooltip_text(message);
		return;
	}

	// Clear results
	result_items.clear();
	results->clear();
	results->create_item();
	current_file_display->set_text("");
	current_file_folder_display->set_text("");

	// Remove editor if search cleared.
	if (search_string.is_empty()) {
		_set_editor(nullptr);
		status_display->set_text(TTR("Type a search query to find in files."));
		return;
	}

	// Start new search.
	searcher->start();
	update_poll_timer->start();
}

void FindInFilesDialog2::_update_search_status() {
	auto status = searcher->get_status();
	String status_string;
	if (status.limit_reached) {
		status_string = vformat("%s+ matches in %s+ files", status.results.size(), status.files_with_matches);
	} else {
		status_string = vformat("%s matches in %s files", status.results.size(), status.files_with_matches);
	}

	status_display->set_text(status_string);

	for (const FindInFilesSearcher::FindResult &r : status.results) {
		String result_id = vformat("%s_%s_%s_%s_%s", r.path, r.start_line, r.start_col, r.end_line, r.end_col);
		if (result_items.has(result_id)) {
			continue;
		}

		TreeItem *item = results->create_item();
		// Do this first because it resets properties of the cell...
		item->set_cell_mode(0, TreeItem::CELL_MODE_CUSTOM);

		String text = r.line_begin_string;
		text = text.strip_edges(true, false);
		// Only draw text up to a limit to prevent slowdown due to long one-liner files.
		if (text.size() > 150) {
			text = text.substr(0, 150) + "...";
		}

		item->set_text(0, text);
		item->set_custom_draw(0, this, "_draw_result_text");
		item->set_metadata(0, r.path);
		item->set_meta("id", result_id);

		result_items[result_id] = r;
	}

	// Select first result when it is available - do not override user selection.
	if (!results->get_selected() && results->get_root() && results->get_root()->get_first_child()) {
		results->get_root()->get_first_child()->select(0);
	}

	if (status.finished) {
		update_poll_timer->stop();
	}
}

void FindInFilesDialog2::_draw_result_text(Object *item_obj, Rect2 rect) {
	TreeItem *item = Object::cast_to<TreeItem>(item_obj);
	if (!item) {
		return;
	}

	const String id = item->get_meta("id", "");
	if (id.is_empty()) {
		return;
	}

	HashMap<String, FindInFilesSearcher::FindResult>::Iterator E = result_items.find(id);
	if (!E) {
		return;
	}
	FindInFilesSearcher::FindResult r = E->value;

	Ref<Font> font = results->get_theme_font(SNAME("font"));
	int font_size = results->get_theme_font_size(SNAME("font_size"));

	int original_size = r.line_begin_string.size();
	int trimmed_size = item->get_text(0).size();

	if (trimmed_size < 150) {
		int start_highlight_col = trimmed_size - original_size + r.start_col;
		int highlight_length = r.start_line != r.end_line ? -1 : r.end_col - r.start_col;

		Rect2 match_rect = rect;
		match_rect.position.x += font->get_string_size(item->get_text(0).left(start_highlight_col), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
		match_rect.size.x = font->get_string_size(item->get_text(0).substr(start_highlight_col, highlight_length), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
		match_rect.position.y += 1 * EDSCALE;
		match_rect.size.y -= 2 * EDSCALE;

		// Use the inverted accent color to help match rectangles stand out even on the currently selected line.
		results->draw_rect(match_rect, get_theme_color(SNAME("accent_color"), SNAME("Editor")).inverted() * Color(1, 1, 1, 0.35f));
	}

	// Filename + line number
	Point2 file_string_pos = Point2(rect.get_end().x, rect.get_position().y);
	const String file_text = vformat("%s: %s", r.path.get_file(), r.start_line + 1);
	const Size2 file_string_size = font->get_string_size(file_text, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size);

	file_string_pos.x -= 2 * EDSCALE + file_string_size.width;
	file_string_pos.y += rect.size.y - file_string_size.y / 2;

	results->draw_string(font, file_string_pos, file_text, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size, Color(1, 1, 1, 0.4f));
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
	const TreeItem *selected = results->get_selected();
	ERR_FAIL_COND_MSG(!selected, "Can't perform replace - nothing selected.");

	String id = selected->get_meta("id");
	ERR_FAIL_COND_MSG(id.is_empty(), "Can't perform replace - selected does not have ID metadata");

	FindInFilesSearcher::FindResult result = result_items[id];
	if (searcher->replace_match(result, replace_line_edit->get_text())) {
		_update_mini_editor();
		_update_selected_item();
		ScriptEditor::get_singleton()->reload_scripts();
	}
}

void FindInFilesDialog2::_do_replace_all() {
	// TBD
}

void FindInFilesDialog2::_update_replace_preview() {
	const TreeItem *selected = results->get_selected();
	ERR_FAIL_COND_MSG(!selected, "Can't perform replace - nothing selected.");

	String id = selected->get_meta("id");
	ERR_FAIL_COND_MSG(id.is_empty(), "Can't perform replace - selected does not have ID metadata");

	FindInFilesSearcher::FindResult result = result_items[id];
	const String preview = searcher->get_replace_match_preview(result, replace_line_edit->get_text());

	replace_preview->set_text(preview);
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
			invalid_result_color = get_theme_color(SNAME("error_color"), SNAME("Editor"));
			match_case_btn->set_icon(get_theme_icon(SNAME("MatchCase"), SNAME("EditorIcons")));
			match_word_btn->set_icon(get_theme_icon(SNAME("MatchWord"), SNAME("EditorIcons")));
			match_regex_btn->set_icon(get_theme_icon(SNAME("MatchRegex"), SNAME("EditorIcons")));

			status_display->add_theme_font_override("font", get_theme_font(SNAME("bold"), SNAME("EditorFonts")));
			current_file_folder_display->add_theme_color_override("font_color", current_file_folder_display->get_theme_color(SNAME("disabled_font_color"), SNAME("Editor")));

			search_validation->set_texture(get_theme_icon("StatusError", "EditorIcons"));
		} break;
	}
}

void FindInFilesDialog2::_bind_methods() {
	ClassDB::bind_method("_draw_result_text", &FindInFilesDialog2::_draw_result_text);
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
	set_ok_button_text("Open in Panel");
	set_process_shortcut_input(true);

	set_min_size(Size2(720 * EDSCALE, 600 * EDSCALE));
	set_title(TTR("Find in Files"));

	searcher = memnew(FindInFilesSearcher);
	searcher->set_result_limit(100);
	update_poll_timer = memnew(Timer);
	update_poll_timer->set_wait_time(0.05);
	update_poll_timer->connect("timeout", callable_mp(this, &FindInFilesDialog2::_update_search_status));
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
	search_line_edit->connect("text_changed", callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	search_hbc->add_child(search_line_edit);

	match_case_btn = memnew(Button);
	match_case_btn->set_flat(true);
	match_case_btn->set_toggle_mode(true);
	match_case_btn->set_tooltip_text(TTR("Match case"));
	match_case_btn->set_focus_mode(Control::FOCUS_CLICK);
	match_case_btn->connect("toggled", callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	search_hbc->add_child(match_case_btn);

	match_word_btn = memnew(Button);
	match_word_btn->set_flat(true);
	match_word_btn->set_toggle_mode(true);
	match_word_btn->set_tooltip_text(TTR("Match whole words (incompatible with Regex)"));
	match_word_btn->set_focus_mode(Control::FOCUS_CLICK);
	match_word_btn->connect("toggled", callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	search_hbc->add_child(match_word_btn);

	match_regex_btn = memnew(Button);
	match_regex_btn->set_flat(true);
	match_regex_btn->set_toggle_mode(true);
	match_regex_btn->set_tooltip_text(TTR("Use regular expressions (regex)"));
	match_regex_btn->set_focus_mode(Control::FOCUS_CLICK);
	match_regex_btn->connect("toggled", callable_mp(this, &FindInFilesDialog2::_on_match_regex_toggled));
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
	folder_line_edit->connect("text_changed", callable_mp(this, &FindInFilesDialog2::_on_folder_text_changed));
	files_filter_hbc->add_child(folder_line_edit);

	folder_dialog = memnew(FileDialog);
	folder_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_DIR);
	folder_dialog->connect("dir_selected", callable_mp(this, &FindInFilesDialog2::_on_folder_selected));
	folder_dialog->set_title(TTR("Select a folder"));
	add_child(folder_dialog);

	Button *folder_btn = memnew(Button);
	folder_btn->set_text("...");
	folder_btn->connect("pressed", callable_mp(folder_dialog, &FileDialog::popup_file_dialog));
	files_filter_hbc->add_child(folder_btn);

	Label *file_filter_label = memnew(Label);
	file_filter_label->set_text("File Filter");
	files_filter_hbc->add_child(file_filter_label);

	file_filter_chkbx = memnew(CheckBox);
	file_filter_chkbx->set_pressed(true);
	file_filter_chkbx->connect("toggled", callable_mp(this, &FindInFilesDialog2::_on_file_filter_toggled));
	files_filter_hbc->add_child(file_filter_chkbx);

	file_filter_line_edit = memnew(LineEdit);
	file_filter_line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	file_filter_line_edit->set_text("*.gd,*.gdshader");
	file_filter_line_edit->set_editable(file_filter_chkbx->is_pressed());
	file_filter_line_edit->connect("text_changed", callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	files_filter_hbc->add_child(file_filter_line_edit);

	recent_file_filters_btn = memnew(MenuButton);
	recent_file_filters_btn->set_text("Recent Filters");
	recent_file_filters_btn->get_popup()->connect("index_pressed", callable_mp(this, &FindInFilesDialog2::_on_recent_file_filter_selected));
	files_filter_hbc->add_child(recent_file_filters_btn);

	// Status
	HBoxContainer *status_hbc = memnew(HBoxContainer);
	main_vbc->add_child(status_hbc);

	status_display = memnew(Label);
	status_display->set_text(TTR("Type a search query to find in files."));
	status_hbc->add_child(status_display);

	Label *replace_preview_label = memnew(Label);
	replace_preview_label->set_text(TTR("After replacement:"));
	status_hbc->add_child(replace_preview_label);

	replace_preview = memnew(Label);
	status_hbc->add_child(replace_preview);

	// The results list & editor
	split = memnew(VSplitContainer);
	split->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbc->add_child(split);

	results = memnew(Tree);
	results->set_stretch_ratio(0.75);
	results->add_theme_font_override("font", EditorNode::get_singleton()->get_gui_base()->get_theme_font(SNAME("source"), SNAME("EditorFonts")));
	results->add_theme_font_size_override("font_size", EditorNode::get_singleton()->get_gui_base()->get_theme_font_size(SNAME("source_size"), SNAME("EditorFonts")));
	results->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	results->connect("item_selected", callable_mp(this, &FindInFilesDialog2::_update_mini_editor));
	results->connect("item_selected", callable_mp(this, &FindInFilesDialog2::_update_replace_preview));
	results->connect("item_activated", callable_mp(this, &FindInFilesDialog2::_on_result_activated));
	results->set_hide_root(true);
	results->set_select_mode(Tree::SELECT_ROW);
	results->set_allow_rmb_select(true);
	results->create_item(); // Root
	split->add_child(results);

	VBoxContainer *bottom_split_vbox = memnew(VBoxContainer);
	bottom_split_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	split->add_child(bottom_split_vbox);

	HBoxContainer *file_display_hbox = memnew(HBoxContainer);
	bottom_split_vbox->add_child(file_display_hbox);

	current_file_display = memnew(Label);
	file_display_hbox->add_child(current_file_display);

	current_file_folder_display = memnew(Label);
	file_display_hbox->add_child(current_file_folder_display);

	editor_container = memnew(PanelContainer);
	editor_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	bottom_split_vbox->add_child(editor_container);

	editor = nullptr;
}
