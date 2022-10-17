/*************************************************************************/
/*  find_in_files.cpp                                                    */
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

#include "find_in_files_2.h"

#include "code_editor.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/string/string_builder.h"
#include "editor_node.h"
#include "editor_paths.h"
#include "editor_scale.h"
#include "editor_settings.h"
#include "plugins/script_editor_plugin.h"
#include "scene/gui/check_box.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/line_edit.h"

void FindInFilesSearcher::_thread_func(void *self) {
	FindInFilesSearcher *searcher = static_cast<FindInFilesSearcher *>(self);
	searcher->_thread_process();
}

void FindInFilesSearcher::_thread_process() {
	SearchInputData input = _create_input_data();
	PackedStringArray filepaths;

	Vector<Ref<RegEx>> allow_regexs;
	for (String allow_string : input.allow_file_regex_strings) {
		Ref<RegEx> re = RegEx::create_from_string(allow_string);
		if (re.is_valid()) {
			allow_regexs.append(re);
		}
	}

	Vector<Ref<RegEx>> ignore_regexs;
	for (String ignore_string : input.ignore_file_regex_strings) {
		Ref<RegEx> re = RegEx::create_from_string(ignore_string);
		if (re.is_valid()) {
			ignore_regexs.append(re);
		}
	}

	if (_is_cancelled()) {
		return;
	}

	_thread_get_files_from_dir(input.directory, allow_regexs, ignore_regexs, filepaths);

	if (_is_cancelled()) {
		return;
	}

	// Create regex to use in searching.
	Ref<RegEx> regex = _get_regex(input.text, input.match_use_regex, input.match_case_sensitive, input.match_whole_words);
	ERR_FAIL_COND_MSG(regex.is_null() || !regex->is_valid(), "Regular expression for search is invalid");

	// Search in files from dir
	Vector<FindResult> results;
	int searched = 0;
	int searched_with_matches = 0;
	bool limit_reached = false;
	for (auto filepath : filepaths) {
		if (_is_cancelled()) {
			break;
		}

		int matches_from_file = _thread_get_matches_from_file(filepath, results, regex);
		searched++;
		searched_with_matches += matches_from_file > 0 ? 1 : 0;

		if (_is_cancelled()) {
			break;
		}

		limit_reached = results.size() > get_result_limit();
		if (limit_reached) {
			break;
		}

		_update_status(false, searched, searched_with_matches, limit_reached, results);
		OS::get_singleton()->delay_usec(15000);
	}

	_update_status(true, searched, searched_with_matches, limit_reached, results);
}

void FindInFilesSearcher::_thread_get_files_from_dir(const String &p_dir_path, const Vector<Ref<RegEx>> &p_allow_regex, const Vector<Ref<RegEx>> &p_ignore_regex, PackedStringArray &r_filepaths) {
	Ref<DirAccess> dir = DirAccess::open(p_dir_path);

	if (dir.is_null()) {
		return;
	}

	dir->list_dir_begin();

	PackedStringArray dir_files;
	PackedStringArray dir_subdirs;

	while (!_is_cancelled()) {
		String name = dir->get_next();
		if (name.is_empty()) {
			break;
		}

		String fullpath = dir->get_current_dir().path_join(name);

		if (name == ".gdignore") {
			dir_files.clear();
			dir_subdirs.clear();
			break;
		}

		// Ignore special directories (such as those beginning with . and the project data directory).
		String project_data_dir_name = ProjectSettings::get_singleton()->get_project_data_dir_name();
		if (name.begins_with(".") || name == project_data_dir_name) {
			continue;
		}

		if (dir->current_is_hidden()) {
			continue;
		}

		if (dir->current_is_dir()) {
			dir_subdirs.push_back(fullpath);
			continue;
		}

		if (!p_allow_regex.is_empty() || !p_ignore_regex.is_empty()) {
			bool allowed = false;
			for (const Ref<RegEx> &allow_regex : p_allow_regex) {
				if (allow_regex->search(name).is_valid()) {
					allowed = true;
					break;
				}
			}
			if (!allowed) {
				continue;
			}

			for (const Ref<RegEx> &ignore_regex : p_ignore_regex) {
				if (ignore_regex->search(name).is_valid()) {
					allowed = false;
					break;
				}
			}

			if (!allowed) {
				continue;
			}
		}

		dir_files.push_back(fullpath);
	}

	r_filepaths.append_array(dir_files);

	for (const String &dir_path : dir_subdirs) {
		_thread_get_files_from_dir(dir_path, p_allow_regex, p_ignore_regex, r_filepaths);
	}
}

int FindInFilesSearcher::_thread_get_matches_from_file(const String &p_path, Vector<FindResult> &p_results, const Ref<RegEx> &p_regex) const {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::ModeFlags::READ);

	if (fa.is_null()) {
		return 0;
	}

	const String file_text = fa->get_as_text(true);
	const PackedStringArray lines = file_text.split("\n");

	TypedArray<RegExMatch> matches = p_regex->search_all(file_text);

	for (int i = 0; i < matches.size(); ++i) {
		if (_is_cancelled()) {
			break;
		}

		Ref<RegExMatch> match = matches[i];
		int match_start_idx = match->get_start(0);
		int match_end_idx = match->get_end(match->get_group_count());

		int start_line = file_text.count("\n", 0, match_start_idx);
		int start_line_start_idx = file_text.rfindn("\n", match_start_idx) + 1;
		int start_col = match_start_idx - start_line_start_idx;

		int end_line = file_text.count("\n", match_start_idx, match_end_idx) + start_line;
		int end_line_start_idx = file_text.rfindn("\n", match_end_idx) + 1;
		int end_col = match_end_idx - end_line_start_idx;

		p_results.push_back(FindResult(p_path, lines[start_line], start_line, start_col, end_line, end_col));
	}

	return matches.size();
}

void FindInFilesSearcher::_update_status(bool p_finished, int p_files_searched, int p_files_with_matches, int p_limit_reached, const Vector<FindResult> &p_results) {
	_THREAD_SAFE_METHOD_
	status.finished = p_finished;
	status.files_searched = p_files_searched;
	status.files_with_matches = p_files_with_matches;
	status.limit_reached = p_limit_reached;
	status.results = p_results;
}

bool FindInFilesSearcher::_is_cancelled() const {
	_THREAD_SAFE_METHOD_
	return is_cancelled;
}

void FindInFilesSearcher::_set_cancelled(bool p_cancelled) {
	_THREAD_SAFE_METHOD_
	is_cancelled = p_cancelled;
}

FindInFilesSearcher::SearchInputData FindInFilesSearcher::_create_input_data() const {
	_THREAD_SAFE_METHOD_
	return SearchInputData(
			text,
			directory,
			allow_regex_strings,
			ignore_regex_strings,
			result_limit,
			match_case_sensitive,
			match_whole_words,
			match_use_regex);
}

String FindInFilesSearcher::_regex_escape(const String &p_string, bool p_escape_asterisk) {
	String str = p_string;
	str = str.replace("\\", "\\\\")
				  .replace(".", "\\.")
				  .replace("[", "\\[")
				  .replace("]", "\\]")
				  .replace("{", "\\{")
				  .replace("}", "\\}")
				  .replace("-", "\\-")
				  .replace("(", "\\(")
				  .replace(")", "\\)")
				  .replace("+", "\\+")
				  .replace("?", "\\?")
				  .replace(",", "\\,")
				  .replace("/", "\\/")
				  .replace("^", "\\^")
				  .replace("$", "\\$")
				  .replace("|", "\\|")
				  .replace("#", "\\#");

	if (p_escape_asterisk) {
		str = str.replace("*", "\\*");
	}

	return str;
}

Ref<RegEx> FindInFilesSearcher::_get_regex(const String &p_text, bool p_text_is_regex, bool p_case_sensitive, bool p_match_words) {
	Ref<RegEx> regex;
	regex.instantiate();

	String use_text;
	if (p_text_is_regex) {
		use_text = p_text;
		if (!p_case_sensitive) {
			use_text = use_text.insert(0, "(?i)");
		}
	} else {
		use_text = _regex_escape(p_text);
		if (p_match_words) {
			// See demo of this logic at regexr.com/70a29
			const String non_space_regex = "[a-zA-Z0-9_]";
			const String neg_lookbehind = vformat("(?<!%s)", non_space_regex);
			const String neg_lookahead = vformat("(?!%s)", non_space_regex);
			use_text = vformat("%s%s%s", neg_lookbehind, use_text, neg_lookahead);
		}
		if (!p_case_sensitive) {
			use_text = use_text.insert(0, "(?i)");
		}
	}

	regex->compile(use_text);
	return regex;
}

void FindInFilesSearcher::_bind_methods() {
}

FindInFilesSearcher::FindInFilesStatus FindInFilesSearcher::get_status() const {
	_THREAD_SAFE_METHOD_
	return status;
}

void FindInFilesSearcher::start() {
	_THREAD_SAFE_METHOD_
	is_cancelled = false;
	status = FindInFilesStatus();

	worker_thread.start(_thread_func, this);
}

void FindInFilesSearcher::stop() {
	{
		// Mark as cancelled in a separate scope so that we don't deadlock with _is_cancelled() in the thread.
		_THREAD_SAFE_METHOD_
		is_cancelled = true;
	}
	// Should be near-immediate as long as _is_cancelled() is checked often in the worker thread.
	worker_thread.wait_to_finish();

	_THREAD_SAFE_METHOD_
	status = FindInFilesStatus();
}

bool FindInFilesSearcher::is_valid() const {
	_THREAD_SAFE_METHOD_
	Ref<RegEx> regex = _get_regex(text, match_use_regex, match_case_sensitive, match_whole_words);

	if (regex.is_null() || !regex->is_valid()) {
		return false;
	}

	return true;
}

void FindInFilesSearcher::set_search_text(const String &p_text) {
	_THREAD_SAFE_METHOD_
	text = p_text;
}

String FindInFilesSearcher::get_search_text() const {
	_THREAD_SAFE_METHOD_
	return text;
}

void FindInFilesSearcher::set_directory(const String &p_directory) {
	_THREAD_SAFE_METHOD_
	directory = p_directory;
}

void FindInFilesSearcher::set_file_filter(const String &p_file_filter) {
	_THREAD_SAFE_METHOD_
	allow_regex_strings.clear();
	ignore_regex_strings.clear();

	if (p_file_filter.is_empty()) {
		return;
	}

	PackedStringArray filters = p_file_filter.split(",");

	for (const String &filter : filters) {
		String str = filter.strip_edges();
		bool is_ignore = str.begins_with("!");
		str = str.lstrip("!");
		str = _regex_escape(str, false);
		str = str.replace("*", ".*");
		str = "^" + str + "$";

		if (is_ignore) {
			ignore_regex_strings.insert(str);
		} else {
			allow_regex_strings.insert(str);
		}
	}
}

void FindInFilesSearcher::set_result_limit(int p_limit) {
	_THREAD_SAFE_METHOD_
	result_limit = p_limit;
}

int FindInFilesSearcher::get_result_limit() const {
	_THREAD_SAFE_METHOD_
	return result_limit;
}

void FindInFilesSearcher::set_case_sensitive(bool p_case_sensitive) {
	_THREAD_SAFE_METHOD_
	match_case_sensitive = p_case_sensitive;
}

bool FindInFilesSearcher::is_case_sensitive() const {
	_THREAD_SAFE_METHOD_
	return match_case_sensitive;
}

void FindInFilesSearcher::set_whole_words(bool p_whole_words) {
	_THREAD_SAFE_METHOD_
	match_whole_words = p_whole_words;
}

void FindInFilesSearcher::set_use_regex(bool p_use_regex) {
	_THREAD_SAFE_METHOD_
	match_use_regex = p_use_regex;
}

FindInFilesSearcher::FindInFilesSearcher() {
}

void FindInFilesDialog2::_bind_methods() {
	ClassDB::bind_method("_draw_result_text", &FindInFilesDialog2::_draw_result_text);
}

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

void FindInFilesDialog2::_on_result_selected() {
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
			editor->goto_line(r.start_line);
		}
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
	if (searcher->is_valid()) {
		search_validation->set_texture(Ref<Texture2D>());
	} else {
		search_validation->set_texture(get_theme_icon("StatusError", "EditorIcons"));
		search_validation->set_tooltip_text(TTR("Search is not valid, please check the regular expression."));
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

	// Add in reverse as newest = last, want newest on top.
	for (int i = recent_filters.size() - 1; i >= 0; --i) {
		popup->add_item(recent_filters[i]);
	}
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
			} else {
				_save_recent_filters(false);
			}
		} break;
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			match_case_btn->set_icon(get_theme_icon(SNAME("MatchCase"), SNAME("EditorIcons")));
			match_word_btn->set_icon(get_theme_icon(SNAME("MatchWord"), SNAME("EditorIcons")));
			match_regex_btn->set_icon(get_theme_icon(SNAME("MatchRegex"), SNAME("EditorIcons")));

			status_display->add_theme_font_override("font", get_theme_font(SNAME("bold"), SNAME("EditorFonts")));
			current_file_folder_display->add_theme_color_override("font_color", current_file_folder_display->get_theme_color(SNAME("disabled_font_color"), SNAME("Editor")));
		} break;
	}
}

FindInFilesDialog2::FindInFilesDialog2() {
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
	match_case_btn->connect("toggled", callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	search_hbc->add_child(match_case_btn);

	match_word_btn = memnew(Button);
	match_word_btn->set_flat(true);
	match_word_btn->set_toggle_mode(true);
	match_word_btn->set_tooltip_text(TTR("Match whole words (incompatible with Regex)"));
	match_word_btn->connect("toggled", callable_mp(this, &FindInFilesDialog2::_run_search).unbind(1));
	search_hbc->add_child(match_word_btn);

	match_regex_btn = memnew(Button);
	match_regex_btn->set_flat(true);
	match_regex_btn->set_toggle_mode(true);
	match_regex_btn->set_tooltip_text(TTR("Use regular expressions (regex)"));
	match_regex_btn->connect("toggled", callable_mp(this, &FindInFilesDialog2::_on_match_regex_toggled));
	search_hbc->add_child(match_regex_btn);

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
	add_child(folder_dialog);

	Button *folder_btn = memnew(Button);
	folder_btn->set_text("...");
	folder_btn->connect("pressed", callable_mp(folder_dialog, &FileDialog::popup_file_dialog));
	files_filter_hbc->add_child(folder_btn);

	Label *file_filter_label = memnew(Label);
	file_filter_label->set_text("File Filter");
	files_filter_hbc->add_child(file_filter_label);

	file_filter_chkbx = memnew(CheckBox);
	file_filter_chkbx->set_pressed(false);
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
	results->connect("item_selected", callable_mp(this, &FindInFilesDialog2::_on_result_selected));
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
