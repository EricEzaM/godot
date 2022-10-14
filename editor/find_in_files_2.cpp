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
#include "editor_scale.h"
#include "plugins/script_editor_plugin.h"
#include "scene/gui/check_box.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"

void FindInFilesSearcher::_thread_func(void *self) {
	FindInFilesSearcher *searcher = static_cast<FindInFilesSearcher *>(self);
	searcher->_thread_process();
}

void FindInFilesSearcher::_thread_process() {
	while (true) {
		worker_thread_wait.wait();
		_reset_cancelled();
		int start_search_id = _get_search_id();

		PackedStringArray filepaths;
		_get_files_from_dir("res://" + directory, allowed_extensions, filepaths);

		if (_is_cancelled()) {
			continue;
		}

		// Search in files from dir
		Vector<FindResult> results;
		int done = 0;
		for (auto filepath : filepaths) {
			if (_is_cancelled()) {
				break;
			}
			_get_matches_from_file(filepath, results);
			done++;
			if (_is_cancelled() || _get_search_id() != start_search_id) {
				break;
			}

			_update_status(vformat("searching files..."), ((float)done / filepaths.size()), results);

			OS::get_singleton()->delay_usec(15000);
		}
	}
}

int FindInFilesSearcher::_get_search_id() const {
	_THREAD_SAFE_METHOD_
	return search_id;
}

bool FindInFilesSearcher::_is_cancelled() const {
	_THREAD_SAFE_METHOD_
	return cancel_flag;
}

void FindInFilesSearcher::_reset_cancelled() {
	_THREAD_SAFE_METHOD_
	cancel_flag = false;
}

void FindInFilesSearcher::_get_files_from_dir(const String &p_dir_path, const HashSet<String> &p_allowed_extensions, PackedStringArray &r_filepaths) {
	Ref<DirAccess> dir = DirAccess::open(p_dir_path);

	if (dir.is_null()) {
		return;
	}

	dir->list_dir_begin();

	PackedStringArray dir_files;
	PackedStringArray dir_subdirs;

	while (true) {
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
		} else {
			String ext = name.get_extension();
			if (p_allowed_extensions.has(ext)) {
				dir_files.push_back(fullpath);
			}
		}
	}

	r_filepaths.append_array(dir_files);
	_update_status(vformat("scanning drive for files, found %s", r_filepaths.size()));

	for (const String &dir_path : dir_subdirs) {
		_get_files_from_dir(dir_path, p_allowed_extensions, r_filepaths);
	}
}

void FindInFilesSearcher::_get_matches_from_file(const String &p_path, Vector<FindResult> &p_results) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::ModeFlags::READ);

	if (fa.is_null()) {
		return;
	}

	const String file_text = fa->get_as_text(true);
	const PackedStringArray lines = file_text.split("\n");

	TypedArray<RegExMatch> matches = regex.search_all(file_text);

	for (int i = 0; i < matches.size(); ++i) {
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
		call_deferred("emit_signal", "result_found", p_path, lines[start_line], start_line, start_col, end_line, end_col);
	}
}

void FindInFilesSearcher::_update_status(const String &p_text, const float p_progress, const Vector<FindResult> &p_results) {
	_THREAD_SAFE_METHOD_
	status.text = p_text;
	status.progress = p_progress;
	status.results = p_results;
}

void FindInFilesSearcher::_bind_methods() {
	ADD_SIGNAL(MethodInfo("result_found",
			PropertyInfo(Variant::STRING, "path"),
			PropertyInfo(Variant::STRING, "start_line_string"),
			PropertyInfo(Variant::INT, "start_line"),
			PropertyInfo(Variant::INT, "start_col"),
			PropertyInfo(Variant::INT, "end_line"),
			PropertyInfo(Variant::INT, "end_col")));
}

FindInFilesSearcher::FindInFilesStatus FindInFilesSearcher::get_status() const {
	_THREAD_SAFE_METHOD_
	return status;
}

void FindInFilesSearcher::start() {
	_THREAD_SAFE_METHOD_
	// cancel_flag = false;
	worker_thread_wait.post();
}

void FindInFilesSearcher::stop() {
	_THREAD_SAFE_METHOD_
	cancel_flag = true;
	status.text = "";
	status.progress = 1;
	status.results = Vector<FindResult>();
}

void FindInFilesSearcher::set_search_text(const String &p_text) {
	text = p_text;
	if (use_regex) {
		regex.compile(p_text);
	} else {
		String result = regex_escape.sub(p_text, "\\$&", true);
		regex.compile(result);
	}
}

FindInFilesSearcher::FindInFilesSearcher() {
	regex_escape.compile("[-[\\]{}()*+?.,\\\\/^$|#\\s]");

	worker_thread.start(_thread_func, this);
	allowed_extensions.insert("gd");
}

void FindInFilesDialog2::_bind_methods() {
	ClassDB::bind_method("_draw_result_text", &FindInFilesDialog2::_draw_result_text);
}

void FindInFilesDialog2::_on_result_found(const String &p_path, const String &p_line_string, int p_start_line, int p_start_col, int p_end_line, int p_end_col) {
	String result_id = vformat("%s_%s_%s_%s_%s", p_path, p_start_line, p_start_col, p_end_line, p_end_col);

	TreeItem *item = results->create_item();
	// Do this first because it resets properties of the cell...
	item->set_cell_mode(0, TreeItem::CELL_MODE_CUSTOM);

	String text = p_line_string;
	text = text.strip_edges(true, false);

	item->set_text(0, text);
	item->set_custom_draw(0, this, "_draw_result_text");
	item->set_metadata(0, p_path);
	item->set_meta("id", result_id);

	result_items[result_id] = FindInFilesSearcher::FindResult(p_path, p_line_string, p_start_line, p_start_col, p_end_line, p_end_col);
}

FindInFilesDialog2::FindInFilesDialog2() {
	set_min_size(Size2(720 * EDSCALE, 500 * EDSCALE));
	set_title(TTR("Find in Files"));

	searcher = memnew(FindInFilesSearcher);
	// searcher->connect("result_found", callable_mp(this, &FindInFilesDialog2::_on_result_found));
	update_poll_timer = memnew(Timer);
	update_poll_timer->set_wait_time(0.01);
	update_poll_timer->connect("timeout", callable_mp(this, &FindInFilesDialog2::_update_search_status));
	add_child(update_poll_timer);

	vbc = memnew(VBoxContainer);
	vbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(vbc);

	status_display = memnew(Label);
	vbc->add_child(status_display);

	search_line_edit = memnew(LineEdit);
	vbc->add_child(search_line_edit);
	search_line_edit->connect("text_changed", callable_mp(this, &FindInFilesDialog2::_on_text_changed));

	split = memnew(VSplitContainer);
	split->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vbc->add_child(split);

	results = memnew(Tree);
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

	Label *placeholder = memnew(Label);
	placeholder->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	placeholder->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	placeholder->set_text("Start typing search query to find in files.");
	split->add_child(placeholder);

	editor = nullptr;
}

void FindInFilesDialog2::_on_text_changed(const String &p_string) {
	// update_poll_timer->stop();
	searcher->stop(); // Cancel existing search.

	result_items.clear();
	results->clear();
	results->create_item(); // Root.

	searcher->set_search_text(p_string);
	searcher->start(); // Start new search.
	update_poll_timer->start();
}

void FindInFilesDialog2::_set_editor(ScriptEditorBase *p_editor) {
	if (split->get_child_count(false) == 2) {
		Node *node = split->get_child(1, false);
		split->remove_child(node);
		node->queue_delete();
	}
	ERR_FAIL_COND_MSG(split->get_child_count(false) != 1, "Split container can only have one child before the editor is added to it!");

	editor = p_editor;
	if (editor) {
		split->add_child(editor);
		editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
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

void FindInFilesDialog2::_update_search_status() {
	auto status = searcher->get_status();
	String status_string = vformat("%s :: %0.2f :: %s results", status.text, status.progress, status.results.size());
	print_line(status_string);
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

		item->set_text(0, text);
		item->set_custom_draw(0, this, "_draw_result_text");
		item->set_metadata(0, r.path);
		item->set_meta("id", result_id);

		result_items[result_id] = r;
	}

	if (status.progress >= 1) {
		update_poll_timer->stop();
		if (!results->get_selected() && results->get_root() && results->get_root()->get_first_child()) {
			results->get_root()->get_first_child()->select(0);
		}
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

	int start_highlight_col = trimmed_size - original_size + r.start_col;
	int highlight_length = r.start_line != r.end_line ? -1 : r.end_col - r.start_col;

	Rect2 match_rect = rect;
	match_rect.position.x += font->get_string_size(item->get_text(0).left(start_highlight_col), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
	match_rect.size.x = font->get_string_size(item->get_text(0).substr(start_highlight_col, highlight_length), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
	match_rect.position.y += 1 * EDSCALE;
	match_rect.size.y -= 2 * EDSCALE;

	// Use the inverted accent color to help match rectangles stand out even on the currently selected line.
	results->draw_rect(match_rect, get_theme_color(SNAME("accent_color"), SNAME("Editor")).inverted() * Color(1, 1, 1, 0.35f));

	// Filename
	Point2 file_string_pos = Point2(rect.get_end().x, rect.get_position().y);
	const String file = r.path.get_file();
	const Size2 file_string_size = font->get_string_size(file, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size);

	file_string_pos.x -= 2 * EDSCALE + file_string_size.width;
	file_string_pos.y += rect.size.y - file_string_size.y / 2;

	results->draw_string(font, file_string_pos, file, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size, Color(1, 1, 1, 0.5f));
}

void FindInFilesDialog2::_notification(int p_what) {
	switch (p_what) {
	}
}
