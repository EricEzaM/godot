/*************************************************************************/
/*  find_in_files_searcher.cpp                                           */
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

#include "find_in_files_searcher.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/object/ref_counted.h"
#include "core/os/os.h"
#include "modules/regex/regex.h"

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
		int match_end_idx = match->get_end(0);

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

bool FindInFilesSearcher::is_valid(String &r_message) const {
	_THREAD_SAFE_METHOD_
	Ref<RegEx> regex = _get_regex(text, match_use_regex, match_case_sensitive, match_whole_words);

	if (regex.is_null() || !regex->is_valid()) {
		r_message = TTR("Regular expression is invalid");
		return false;
	}

	if (regex->search("").is_valid()) {
		r_message = TTR("Regular expression matches an empty string");
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
