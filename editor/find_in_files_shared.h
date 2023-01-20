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

#include "find_in_files_searcher.h"
#include "scene/gui/box_container.h"
#include "scene/gui/panel.h"

class ScriptEditorBase;
class PanelContainer;
struct Rect2;
class TreeItem;
class Tree;

class FindInFilesContainerBase : public Control {
	GDCLASS(FindInFilesContainerBase, Control);
};

class FindInFilesEditor : public VBoxContainer {
	GDCLASS(FindInFilesEditor, VBoxContainer);

	ScriptEditorBase *editor = nullptr;

	Label *current_file_display;
	Label *current_file_folder_display;
	Panel *editor_panel;

	void _set_editor(ScriptEditorBase *p_editor);

protected:
	void _notification(int p_what);

public:
	void clear_file();
	void open_file(String p_path, int p_line);

	FindInFilesEditor();
};

void draw_find_result_tree_item(Tree *p_tree, const TreeItem *p_item, Rect2 p_rect, FindInFilesSearcher::FindResult p_result, bool p_draw_line_only);

#endif // FIND_IN_FILES_SHARED_H
