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
#include "scene/gui/dialogs.h"

class FindInFilesPanel2;
class FindInFilesDialog2;
class FindInFilesTree;
class FindInFilesFilePreview;
class TreeItem;
class Button;
class PanelContainer;
class TabContainer;
class Tree;

class FindInFilesPanelTab : public Control {
	GDCLASS(FindInFilesPanelTab, Control)
	friend class FindInFilesPanel2;

	Ref<Texture2D> file_icon;
	Ref<Texture2D> folder_icon;
	Color folder_icon_color;

	FindInFilesDialog2 *dialog;
	FindInFilesSearcher *searcher;
	Timer *update_poll_timer;

	// Maps directory/file to treeitem which represents directory/file.
	HashMap<String, TreeItem *> result_filesystem_levels;
	HashMap<String, FindInFilesSearcher::FindResult> result_items;

	Label *status_display;

	FindInFilesTree *results;
	FindInFilesFilePreview *file_preview;

	ConfirmationDialog *continue_confirm_dialog;

	void _on_dialog_confirmed();
	void _on_open_file_requested(const String &p_path, int p_line, int p_column);

	void _update_from_searcher();
	void _update_file_preview();

	void _soft_limit_continue_search();
	void _soft_limit_cancel_search();
	void _run_search();

protected:
	void _notification(int p_what);

public:
	void expand_collapse_tree(bool p_collapse);
	void popup_configure();
	void rerun_search();

	FindInFilesPanelTab(const FindInFilesSearcher::InputData &p_input_data, const FindInFilesSearcher::Status &p_status);
	~FindInFilesPanelTab();
};

class FindInFilesPanel2 : public Control {
	GDCLASS(FindInFilesPanel2, Control)

	TabContainer *tabs;

	FindInFilesPanelTab *configuring_tab = nullptr;

	Button *refresh_btn;
	Button *configure_btn;
	Button *expand_all_btn;
	Button *collapse_all_btn;

	void _on_tab_changed(int p_new_tab);
	void _on_tab_closed(int p_tab);
	void _on_rerun_pressed();
	void _on_configure_pressed();
	void _on_configure_cancelled();

	void _expand_collapse_tree(bool p_collapse);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void update_search(const FindInFilesSearcher::InputData &p_input_data);
	void add_tab(FindInFilesDialog2 *p_dialog);

	FindInFilesPanel2();
};

#endif // FIND_IN_FILES_PANEL_H
