/*************************************************************************/
/*  find_in_files_shared.h                                               */
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

#ifndef FIND_IN_FILES_SHARED_H
#define FIND_IN_FILES_SHARED_H

#include "scene/gui/box_container.h"

class ScriptEditorBase;
class PanelContainer;
class TreeItem;
class Tree;

struct FindConfiguration {
	String text;

	String file_filter;
	String directory;

	bool match_case_sensitive;
	bool match_whole_words;
	bool match_use_regex;

	FindConfiguration() {
	}

	FindConfiguration(const String &p_text, const String &p_directory, const String &p_file_filter, bool p_case_sensitive, bool p_whole_words, bool p_use_regex) :
			text(p_text),
			file_filter(p_file_filter),
			directory(p_directory),
			match_case_sensitive(p_case_sensitive),
			match_whole_words(p_whole_words),
			match_use_regex(p_use_regex) {
	}
};

struct FindReplaceConfiguration : FindConfiguration {
	bool is_replace_mode;
	String replace_text;

	FindReplaceConfiguration() {
	}

	FindReplaceConfiguration(const FindConfiguration &p_find_configuration, bool p_is_replace_mode, const String &p_replace_text) :
			FindConfiguration(p_find_configuration),
			is_replace_mode(p_is_replace_mode),
			replace_text(p_replace_text) {
	}

	FindReplaceConfiguration(const String &p_text, const String &p_folder_filter, const String &p_file_filter, bool p_case_sensitive, bool p_whole_words, bool p_use_regex, bool p_is_replace_mode, const String &p_replace_text) :
			FindConfiguration(p_text, p_folder_filter, p_file_filter, p_case_sensitive, p_whole_words, p_use_regex),
			is_replace_mode(p_is_replace_mode),
			replace_text(p_replace_text) {
	}

	bool operator==(const FindReplaceConfiguration &p_other) const {
		return text == p_other.text &&
				file_filter == p_other.file_filter &&
				directory == p_other.directory &&
				match_case_sensitive == p_other.match_case_sensitive &&
				match_whole_words == p_other.match_whole_words &&
				match_use_regex == p_other.match_use_regex &&
				is_replace_mode == p_other.is_replace_mode &&
				replace_text == p_other.replace_text;
	}
};

class FindInFilesFilePreview : public VBoxContainer {
	GDCLASS(FindInFilesFilePreview, VBoxContainer)

	ScriptEditorBase *editor = nullptr;

	Label *current_file_display;
	Label *current_file_folder_display;
	Panel *editor_panel;

	void _set_editor(ScriptEditorBase *p_editor);

protected:
	void _notification(int p_what);

public:
	void clear_file();
	void open_file(const String &p_path, int p_line);

	FindInFilesFilePreview();
	~FindInFilesFilePreview();
};

#endif // FIND_IN_FILES_SHARED_H
