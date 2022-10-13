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
		String text;
		float progress;
		Vector<FindResult> results;
	};

private:
	bool cancel_flag;

	FindInFilesStatus status;
	Semaphore worker_thread_wait;
	Thread worker_thread;

	String text;
	String directory;

	bool case_sensitive;
	bool whole_words;
	bool use_regex;

	RegEx regex_escape;
	RegEx regex;
	HashSet<String> allowed_extensions;

	static void _thread_func(void *self);
	void _thread_process();

	bool _is_cancelled() const;
	void _reset_cancelled();

	void _get_files_from_dir(const String &p_dir_path, const HashSet<String> &p_allowed_extensions, PackedStringArray &r_filepaths);
	void _get_matches_from_file(const String &p_path, Vector<FindResult> &p_results);

	void _update_status(const String &p_text, const float p_progress = 0.0f, const Vector<FindResult> &p_results = Vector<FindResult>());

protected:
	static void _bind_methods();

public:
	void set_search_text(const String &p_text);

	void set_case_sensitive(bool p_case_sensitive);
	bool is_case_sensitive() const;

	void set_whole_words(bool p_whole_words);
	bool is_whole_words() const;

	void set_use_regex(bool p_use_regex);
	bool is_using_regex() const;

	void start();
	void stop();

	FindInFilesStatus get_status() const;

	FindInFilesSearcher();
};

class FindInFilesDialog2 : public AcceptDialog {
	GDCLASS(FindInFilesDialog2, AcceptDialog);

private:
	HashMap<String, FindInFilesSearcher::FindResult> result_items;

	VBoxContainer *vbc;
	VSplitContainer *split;
	ScriptEditorBase *editor;
	Tree *results;
	Timer *update_poll_timer;
	Label *status_display;
	LineEdit *search_line_edit;
	FindInFilesSearcher *searcher;

	void _update_search_status();
	void _on_text_changed(const String &p_string);

	void _set_editor(ScriptEditorBase *p_editor);

	void _on_result_selected();
	void _on_result_activated();

	void _draw_result_text(Object *item_obj, Rect2 rect);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	enum FindInFilesMode {
		SEARCH_MODE,
		REPLACE_MODE
	};

	void _on_result_found(const String &p_path, const String &p_line_string, int p_start_line, int p_start_col, int p_end_line, int p_end_col);
	FindInFilesDialog2();
};

#endif // FIND_IN_FILES_2_H
