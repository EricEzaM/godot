/*************************************************************************/
/*  find_in_files_panel.h                                                */
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

#ifndef FIND_IN_FILES_PANEL_H
#define FIND_IN_FILES_PANEL_H

#include "editor/find_in_files_searcher.h"

#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/menu_button.h"

class FindInFilesEditor;
class TreeItem;
class Button;
class PanelContainer;
class TabContainer;
class Tree;

class FindInFilesPanelTab : public Control {
	GDCLASS(FindInFilesPanelTab, Control);

public:
	enum GroupingModeFlags {
		FILE = 1 << 1,
		DIRECTORY = 1 << 2,
	};

private:
	int grouping_mode = FILE | DIRECTORY;

	Ref<Texture2D> folder_icon;
	Ref<Texture2D> file_icon;

	FindInFilesSearcher *searcher;
	Timer *update_poll_timer;

	// Maps directory/file to treeitem which represents directory/file.
	HashMap<String, TreeItem *> result_filesystem_levels;
	HashMap<String, FindInFilesSearcher::FindResult> result_items;

	Label *status_display;

	Tree *results;
	FindInFilesEditor *editor_panel;

	ConfirmationDialog *continue_confirm_dialog;

	void _on_result_activated();

	void _reset();
	void _update_search_status();
	void _update_searcher(FindInFilesSearcher::SearchInputData p_input_data);
	void _update_editor();

	void _soft_limit_continue_search();
	void _soft_limit_cancel_search();
	void _draw_result_text(Object *p_item_obj, Rect2 p_rect);

protected:
	static void _bind_methods();

	void _notification(int p_what);
	void _run_search();

public:
	void expand_collapse_tree(bool p_collapse);

	void set_grouping_mode(int p_mode);
	int get_grouping_mode() const;

	FindInFilesPanelTab(FindInFilesSearcher::SearchInputData p_input_data);
};

class FindInFilesPanel2 : public Control {
	GDCLASS(FindInFilesPanel2, Control);

	static FindInFilesPanel2 *singleton;

	TabContainer *tabs;

	Button *refresh_btn;
	Button *configure_btn;
	Button *group_directory_btn;
	Button *group_files_btn;
	Button *expand_all_btn;
	Button *collapse_all_btn;
	Button *show_source_btn;

	void _on_tab_button_pressed(int p_tab);
	void _expand_collapse_tree(bool p_collapse);
	void _toggle_grouping(bool p_toggled_on, FindInFilesPanelTab::GroupingModeFlags flag);

protected:
	void _notification(int p_what);

public:
	static FindInFilesPanel2 *FindInFilesPanel2::get_singleton() { return singleton; }

	void add_search(FindInFilesSearcher::SearchInputData p_input_data);

	FindInFilesPanel2();
};

VARIANT_ENUM_CAST(FindInFilesPanelTab::GroupingModeFlags)

#endif // FIND_IN_FILES_PANEL_H
