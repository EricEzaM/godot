/*************************************************************************/
/*  find_in_files_dialog.h                                               */
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

#ifndef FIND_IN_FILES_DIALOG_H
#define FIND_IN_FILES_DIALOG_H

#include "editor/find_in_files_searcher.h"
#include "scene/gui/dialogs.h"

class FindInFilesTree;
class TextureRect;
class MenuButton;
class FindInFilesFilePreview;
class LineEdit;
class CheckBox;
class FileDialog;
class HBoxContainer;

class FindInFilesDialog2 : public AcceptDialog {
	GDCLASS(FindInFilesDialog2, AcceptDialog)
public:
	enum FindInFilesMode {
		FIND_MODE,
		REPLACE_MODE
	};

private:
	FindInFilesMode mode = FIND_MODE;
	bool has_changed = false;
	bool run_search_on_popup = true;
	bool clear_results_on_next_update = false;

	Vector<String> recent_filters; // Start = oldest, End = newest
	HashMap<String, FindInFilesSearcher::FindResult> result_items;

	TextureRect *search_validation;
	LineEdit *search_line_edit;
	Button *match_case_btn;
	Button *match_word_btn;
	Button *match_regex_btn;

	HBoxContainer *replace_hbc;
	LineEdit *replace_line_edit;

	LineEdit *folder_line_edit;
	String previous_folder_selection = "res://";
	FileDialog *folder_dialog;
	LineEdit *file_filter_line_edit;
	CheckBox *file_filter_chkbx;
	MenuButton *recent_file_filters_btn;

	Label *status_display;
	Label *replace_preview_label;
	Label *replace_preview;

	FindInFilesTree *results;
	FindInFilesFilePreview *file_preview;
	Color invalid_result_color = Color(1, 0, 0);

	Timer *update_poll_timer;
	FindInFilesSearcher *searcher;

	Button *replace_dialog_btn = nullptr;
	Button *replace_all_dialog_btn = nullptr;

	void _run_search();
	void _clear_results();

	void _update_from_searcher();
	void _select_first_result();

	void _on_folder_selected(const String &path);
	void _on_folder_text_changed(const String &p_string);
	void _on_file_filter_toggled(bool p_toggled_on);
	void _on_recent_file_filter_selected(int p_idx);
	void _on_match_regex_toggled(bool p_toggled_on);
	void _on_search_gui_input(const Ref<InputEvent> &p_input);

	void _update_file_preview();
	void _on_open_file_requested(const String &p_path, int p_line, int p_column);

	void _on_mode_changed();

	void _save_recent_filters(bool p_save_to_editor_cfg);
	void _load_recent_filters();
	void _update_recent_filters_menu();

	void _do_replace_on_selected();
	void _do_replace_all();
	void _update_replace_preview();

	void _set_changed();

protected:
	void _notification(int p_what);

	void custom_action(const String &) override;

public:
	FindInFilesMode get_dialog_mode() const;
	void set_dialog_mode(FindInFilesMode p_mode);

	void shortcut_input(const Ref<InputEvent> &p_event) override;
	void get_initial_search_data(FindInFilesSearcher::InputData &r_input_data, FindInFilesSearcher::Status &r_status);
	// This method sets the initial state of the dialog, for use when "configuring" an existing search from the panel.
	// It means that when the dialog is opened, it will be correctly configured and also have all the results from
	// the previous search so it won't have to re-do the search if no parameters have changed.
	void set_initial_search_data(const FindInFilesSearcher::InputData &p_input_data, const FindInFilesSearcher::Status &p_status);

	void set_find_text(const String &p_text);

	void set_run_search_on_popup(bool p_run);

	bool has_configuration_changed_since_open() const { return has_changed; }

	FindInFilesDialog2();
};

#endif // FIND_IN_FILES_2_H
