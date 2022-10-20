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

#include "code_editor.h"
#include "find_in_files_searcher.h"
#include "modules/regex/regex.h"
#include "plugins/script_editor_plugin.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"

class LineEdit;
class CheckBox;
class FileDialog;
class HBoxContainer;
class FindInFilesSearcher;
struct FindInFilesSearcher::FindResult;

class FindInFilesDialog2 : public AcceptDialog {
	GDCLASS(FindInFilesDialog2, AcceptDialog);

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
	Label *replace_preview;
	Label *current_file_display;
	Label *current_file_folder_display;

	VSplitContainer *split;
	Tree *results;
	PanelContainer *editor_container;
	ScriptEditorBase *editor;

	Timer *update_poll_timer;
	FindInFilesSearcher *searcher;

	Button *replace_dialog_btn = nullptr;
	Button *replace_all_dialog_btn = nullptr;

	void _run_search();

	void _update_search_status();

	void _on_folder_selected(const String &path);
	void _on_folder_text_changed(const String &p_string);
	void _on_file_filter_toggled(bool p_toggled_on);
	void _on_recent_file_filter_selected(int p_idx);
	void _on_match_regex_toggled(bool p_toggled);

	void _on_result_selected();
	void _on_result_activated();

	void _on_mode_changed();

	void _set_editor(ScriptEditorBase *p_editor);

	void _draw_result_text(Object *item_obj, Rect2 rect);

	void _save_recent_filters(bool p_save_to_editor_cfg);
	void _load_recent_filters();
	void _update_recent_filters_menu();

	void _do_replace_on_selected();
	void _do_replace_all();
	void _update_replace_preview();

protected:
	void _notification(int p_what);
	static void _bind_methods();

	void custom_action(const String &) override;

public:
	enum FindInFilesMode {
		FIND_MODE,
		REPLACE_MODE
	} mode;

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

	// Not a great name but using `get_mode()` hides the method of the same name on Window
	FindInFilesMode get_dialog_mode() const;
	void set_find_in_files_mode(FindInFilesMode p_mode);

	FindInFilesDialog2();
};

#endif // FIND_IN_FILES_2_H
