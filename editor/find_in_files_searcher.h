/*************************************************************************/
/*  find_in_files_searcher.h                                             */
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

#ifndef FIND_IN_FILES_SEARCHER_H
#define FIND_IN_FILES_SEARCHER_H

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/os/semaphore.h"
#include "core/os/thread.h"

class RegEx;
class ClassDB;
struct FindConfiguration;

class FindInFilesSearcher : public Object {
	GDCLASS(FindInFilesSearcher, Object)
	_THREAD_SAFE_CLASS_

public:
	struct FindResult {
		String id = "";

		String path = "";
		String line_begin_string = "";

		int start_line = 0;
		int start_col = 0;

		int end_line = 0;
		int end_col = 0;

		int start_in_file = 0;
		int end_in_file = 0;

		FindResult() = default;

		FindResult(const String &p_path, const String &p_line_string, int p_start_line, int p_start_col, int p_end_line, int p_end_col, int p_start_in_file, int p_end_in_file) :
				id(vformat("%s_%s_%s_%s_%s", p_path, p_start_line, p_start_col, p_end_line, p_end_col)),
				path(p_path),
				line_begin_string(p_line_string),
				start_line(p_start_line),
				start_col(p_start_col),
				end_line(p_end_line),
				end_col(p_end_col),
				start_in_file(p_start_in_file),
				end_in_file(p_end_in_file) {
		}

		bool operator==(const FindResult &p_other) const {
			return path == p_other.path &&
					line_begin_string == p_other.line_begin_string &&
					start_line == p_other.start_line &&
					start_col == p_other.start_col &&
					end_line == p_other.end_line &&
					end_col == p_other.end_col;
		}
	};

	struct Status {
		bool finished = false;
		int files_searched = 0;
		int files_with_matches = 0;
		bool limit_reached = false;
		bool soft_limit = false;
		Vector<FindResult> results;
	};

private:
	bool is_cancelled = false;

	Status status;
	Thread worker_thread;

	String text;
	String directory;
	String file_filter;

	int result_limit = 0;
	bool is_result_limit_soft = false;
	bool match_case_sensitive = false;
	bool match_whole_words = false;
	bool match_use_regex = false;

	bool soft_limit_continue = false;
	Semaphore soft_limit_sem;

	static void _thread_func(void *self);
	void _thread_process();

	void _thread_get_files_from_dir(const String &p_dir_path, const Vector<Ref<RegEx>> &p_allow_regex, const Vector<Ref<RegEx>> &p_ignore_regex, PackedStringArray &r_filepaths);
	int _thread_get_matches_from_file(const String &p_path, Vector<FindResult> &p_results, const Ref<RegEx> &p_regex) const;

	void _update_status(bool p_finished, int p_files_searched, int p_files_with_matches, int p_limit_reached, bool p_soft_limit, const Vector<FindResult> &p_results = Vector<FindResult>());

	bool _is_cancelled() const;
	void _set_cancelled(bool p_cancelled);

	bool _is_result_valid(const FindResult &p_result, const Ref<RegEx> &p_regex) const;

	static String _regex_escape(const String &p_string, bool p_escape_asterisk = true);
	static Ref<RegEx> _get_regex(const String &p_text, bool p_text_is_regex, bool p_case_sensitive, bool p_match_words);

protected:
	static void _bind_methods();

public:
	// Batch configuration as one structure, so that changes to any options only affect new searches,
	// not ongoing ones (if the user of this class does not immediately restart the search).
	FindConfiguration get_configuration() const;
	void set_configuration(const FindConfiguration &p_config);

	Status get_status() const;
	void set_status(const Status &p_status);

	String get_replace_match_preview(const FindResult &p_result, const String &p_replacement) const;
	bool replace_match(const FindResult &p_result, const String &p_replacement) const;

	bool is_result_valid(const FindResult &p_result) const;

	void start();
	void stop();

	bool is_valid(String &r_message) const;

	void set_search_text(const String &p_text);
	String get_search_text() const;

	void set_directory(const String &p_directory);
	void set_file_filter(const String &p_file_filter);

	int get_result_limit() const;
	int is_soft_result_limit() const;

	void set_case_sensitive(bool p_case_sensitive);
	bool is_case_sensitive() const;

	void set_whole_words(bool p_whole_words);
	bool is_whole_words() const;

	void set_use_regex(bool p_use_regex);
	bool is_using_regex() const;

	void release_soft_limit(bool p_continue_search);

	FindInFilesSearcher(int p_result_limit, bool p_is_soft_limit);
	~FindInFilesSearcher();
};

#endif // FIND_IN_FILES_SEARCHER_H
