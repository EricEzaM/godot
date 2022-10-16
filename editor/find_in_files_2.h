/*************************************************************************/
/*  find_in_files.h                                                      */
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

#ifndef FIND_IN_FILES_2_H
#define FIND_IN_FILES_2_H

#include "code_editor.h"
#include "modules/regex/regex.h"
#include "plugins/script_editor_plugin.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"

class LineEdit;
class CheckBox;
class FileDialog;
class HBoxContainer;

class FindInFilesSearcher : public Object {
	GDCLASS(FindInFilesSearcher, Object);
	_THREAD_SAFE_CLASS_
public:
	struct FindResult {
		String path = "";

		String line_begin_string = "";

		int start_line = 0;
		int start_col = 0;

		int end_line = 0;
		int end_col = 0;

		FindResult() {}

		FindResult(const String &p_path, const String &p_line_string, int p_start_line, int p_start_col, int p_end_line, int p_end_col) :
				path(p_path), line_begin_string(p_line_string), start_line(p_start_line), start_col(p_start_col), end_line(p_end_line), end_col(p_end_col) {}
	};

	struct FindInFilesStatus {
		bool finished = false;
		int files_searched = 0;
		int files_with_matches = 0;
		bool limit_reached = false;
		Vector<FindResult> results = Vector<FindResult>();
	};

private:
	bool is_cancelled;

	FindInFilesStatus status;
	Thread worker_thread;

	String text;
	String directory;

	int result_limit;
	bool case_sensitive;
	bool whole_words;
	bool use_regex;

	RegEx regex;
	HashSet<String> allow_regex_strings;
	HashSet<String> ignore_regex_strings;

	static void _thread_func(void *self);
	void _thread_process();

	void _thread_get_files_from_dir(const String &p_dir_path, const Vector<Ref<RegEx>> &p_allow_regex, const Vector<Ref<RegEx>> &p_ignore_regex, PackedStringArray &r_filepaths);
	int _thread_get_matches_from_file(const String &p_path, Vector<FindResult> &p_results) const;

	void _update_status(bool p_finished, int p_files_searched, int p_files_with_matches, int p_limit_reached, const Vector<FindResult> &p_results = Vector<FindResult>());

	bool _is_cancelled() const;
	void _set_cancelled(bool p_cancelled);

	static String _regex_escape(const String &p_string, bool p_escape_asterisk = true);

protected:
	static void _bind_methods();

public:
	FindInFilesStatus get_status() const;

	void start();
	void stop();

	void set_search_text(const String &p_text);

	void set_directory(const String &p_directory);
	void set_file_filter(const String &p_file_filter);

	void set_result_limit(int p_limit);
	int get_result_limit() const;

	void set_case_sensitive(bool p_case_sensitive);
	bool is_case_sensitive() const;

	void set_whole_words(bool p_whole_words);
	bool is_whole_words() const;

	void set_use_regex(bool p_use_regex);
	bool is_using_regex() const;

	FindInFilesSearcher();
};

class FindInFilesDialog2 : public AcceptDialog {
	GDCLASS(FindInFilesDialog2, AcceptDialog);

	Vector<String> recent_filters; // Start = oldest, End = newest
	HashMap<String, FindInFilesSearcher::FindResult> result_items;

	LineEdit *search_line_edit;
	Button *match_case_btn;
	Button *match_word_btn;
	Button *match_regex_btn;

	LineEdit *folder_line_edit;
	String previous_folder_selection = "res://";
	FileDialog *folder_dialog;
	LineEdit *file_filter_line_edit;
	CheckBox *file_filter_chkbx;
	MenuButton *recent_file_filters_btn;

	Label *status_display;
	Label *current_file_display;
	Label *current_file_folder_display;

	VSplitContainer *split;
	Tree *results;
	PanelContainer *editor_container;
	ScriptEditorBase *editor;

	Timer *update_poll_timer;
	FindInFilesSearcher *searcher;

	void _run_search();

	void _update_search_status();

	void _on_folder_selected(const String &path);
	void _on_folder_text_changed(const String &p_string);
	void _on_file_filter_toggled(bool p_toggled_on);
	void _on_recent_file_filter_selected(int p_idx);

	void _set_editor(ScriptEditorBase *p_editor);

	void _on_result_selected();
	void _on_result_activated();

	void _draw_result_text(Object *item_obj, Rect2 rect);

	void _save_recent_filters(bool p_save_to_editor_cfg);
	void _load_recent_filters();
	void _update_recent_filters_menu();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	enum FindInFilesMode {
		SEARCH_MODE,
		REPLACE_MODE
	};

	FindInFilesDialog2();
};

#endif // FIND_IN_FILES_2_H
