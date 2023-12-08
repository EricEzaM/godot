/*************************************************************************/
/*  find_in_files_tree.h                                               */
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

#ifndef FIND_IN_FILES_TREE_H
#define FIND_IN_FILES_TREE_H
#include "core/object/object.h"
#include "core/templates/oa_hash_map.h"
#include "find_in_files_searcher.h"
#include "scene/gui/tree.h"

class FindInFilesTree : public Tree {
	GDCLASS(FindInFilesTree, Tree)
	Ref<Texture2D> file_icon;
	Ref<Texture2D> folder_icon;
	Color folder_icon_color;

	bool dialog_mode = false;
	bool group_results_on_same_line = false;

	HashMap<String, TreeItem *> filesystem_items;
	HashMap<String, FindInFilesSearcher::FindResult> result_id_map;

	void _remake_empty_tree_structure();

	TreeItem *_get_result_item_parent(const FindInFilesSearcher::FindResult &p_result);
	void _draw_result_text(Object *p_item_obj, Rect2 p_rect);
	void _draw_find_result_tree_item(const TreeItem *p_item, Rect2 p_rect, List<FindInFilesSearcher::FindResult> *p_results);

	void _on_result_activated();
	void _create_result_item(const FindInFilesSearcher::FindResult &p_result, const String &p_result_id);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void reset();
	void remake_tree();
	void add_result(const FindInFilesSearcher::FindResult &p_result);

	void select_first_non_root() const;

	bool get_group_results_on_same_line() const;
	void set_group_results_on_same_line(bool p_group);

	FindInFilesTree(bool p_dialog_mode);
};

#endif // FIND_IN_FILES_2_H
