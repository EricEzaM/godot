/*************************************************************************/
/*  find_in_files_tree.cpp                                             */
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

#include "editor/find_in_files_tree.h"

#include "editor/editor_scale.h"
#include "editor/editor_string_names.h"
#include "editor/find_in_files_shared.h"
#include "scene/theme/theme_db.h"

void FindInFilesTree::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			file_icon = get_editor_theme_icon(SNAME("File"));
			folder_icon = get_editor_theme_icon(SNAME("Folder"));
		} break;
		case NOTIFICATION_READY: {
			connect(SNAME("item_activated"), callable_mp(this, &FindInFilesTree::_on_result_activated));

			if (dialog_mode) {
				add_theme_font_override("font", get_theme_font(SNAME("source"), EditorStringName(EditorFonts)));
				add_theme_font_size_override("font_size", get_theme_font_size(SNAME("source_size"), EditorStringName(EditorFonts)));
			}

			folder_icon_color = get_theme_color(SNAME("folder_icon_color"), SNAME("FileDialog"));
		} break;
	}
}

void FindInFilesTree::_bind_methods() {
	ClassDB::bind_method("_draw_result_text", &FindInFilesTree::_draw_result_text);
	ADD_SIGNAL(MethodInfo("open_file_requested", PropertyInfo(Variant::STRING, "file_path"), PropertyInfo(Variant::INT, "line")));
}

void FindInFilesTree::select_first_non_root() const {
	if (get_root() && get_root()->get_first_child()) {
		get_root()->get_first_child()->select(0);
	}
}

void FindInFilesTree::remake_tree() {
	_remake_empty_tree_structure();

	for (KeyValue<String, FindInFilesSearcher::FindResult> &element : result_id_map) {
		_create_result_item(element.value, element.key);
	}
}

void FindInFilesTree::add_result(const FindInFilesSearcher::FindResult &p_result) {
	if (result_id_map.has(p_result.id)) {
		return;
	}

	result_id_map[p_result.id] = p_result;
	_create_result_item(p_result, p_result.id);
}

void FindInFilesTree::_create_result_item(const FindInFilesSearcher::FindResult &p_result, const String &p_result_id) {
	// Get the parent item on which the result item would be made a child
	TreeItem *parent = _get_result_item_parent(p_result);
	if (!parent) {
		parent = get_root();
	}

	if (group_results_on_same_line) {
		// Test if there is already an item which represents this line
		const TreeItem *result_on_same_line = nullptr;
		for (int i = 0; i < parent->get_child_count(); ++i) {
			const TreeItem *search_result_item = parent->get_child(i);
			if ((int)search_result_item->get_meta("line") == p_result.start_line && (String)search_result_item->get_meta("path") == p_result.path) {
				result_on_same_line = search_result_item;
				break;
			}
		}

		// Result is on the same line as an existing result that has a tree item.
		// Add this result id to the list of results that tree item represents.
		if (result_on_same_line) {
			Array ids = result_on_same_line->get_meta("ids", Array());
			ERR_FAIL_COND_MSG(ids.is_empty(), "Existsing result had array size of zero, something went wrong!");
			ids.push_back(p_result_id);
			return;
		}
	}

	TreeItem *item = create_item(parent);

	// Do this first because it resets properties of the cell...
	item->set_cell_mode(0, TreeItem::CELL_MODE_CUSTOM);

	String text = p_result.line_begin_string;
	const String trimmed_text = text.strip_edges(true, false);

	text = trimmed_text;
	int text_start_column = 0;
	// Only draw text up to a limit to prevent long horizontal scrolling or slowdown due to long lines.
	// Ensure that the result is still shown even if it occurs at the end of a very long line.
	if (p_result.start_col > item_character_limit / 2) {
		text = text.substr(p_result.start_col - item_character_limit / 2);
		if (text.size() > item_character_limit) {
			text = text.substr(0, item_character_limit) + "...";
		}
		text_start_column = p_result.start_col - item_character_limit / 2;
	} else if (text.size() > item_character_limit) {
		text = text.substr(0, item_character_limit) + "...";
	}

	item->set_text(0, text);
	if (dialog_mode) {
		item->set_custom_font_size(1, get_theme_font_size(SNAME("font_size")) * 0.8);
		item->set_custom_color(1, secondary_font_color);
		item->set_text(1, p_result.path.get_file());
		item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);
	}
	item->set_metadata(0, p_result.path);

	Array ids = Array();
	ids.push_back(p_result_id);
	item->set_meta("ids", ids);
	item->set_meta("trim_count", p_result.line_begin_string.size() - trimmed_text.size());
	item->set_meta("display_text_start_column", text_start_column);
	item->set_meta("path", p_result.path);
	item->set_meta("line", p_result.start_line);
	item->set_meta("column", p_result.start_col);
	item->set_custom_draw(0, this, "_draw_result_text");
}

bool FindInFilesTree::get_group_results_on_same_line() const {
	return group_results_on_same_line;
}

void FindInFilesTree::set_group_results_on_same_line(bool p_group) {
	group_results_on_same_line = p_group;
	remake_tree();
}

TreeItem *FindInFilesTree::_get_result_item_parent(const FindInFilesSearcher::FindResult &p_result) {
	if (dialog_mode) {
		return nullptr;
	}

	// Grouping by directory and file, parent for the item is the tree item representing the file.
	String path_no_res = p_result.path.substr(6); // The path without res://
	const int slice_count = path_no_res.get_slice_count("/");
	String base_path = "";

	for (int i = 0; i < slice_count; ++i) {
		String slice = path_no_res.get_slicec('/', i);

		// Build up the path gradually from the slices to make the tree structure
		String current_path = base_path + (i == 0 ? "" : "/") + slice;

		// Create the tree item if it does not exist
		if (!filesystem_items.has(current_path)) {
			TreeItem *item = filesystem_items.has(base_path) ? create_item(filesystem_items[base_path]) : create_item();
			item->set_text(0, slice);

			// If it's the last slice, it's the file, otherwise its a folder.
			if (i == slice_count - 1) {
				item->set_icon(0, file_icon);
				item->set_tooltip_text(0, p_result.path);
			} else {
				item->set_icon(0, folder_icon);
				item->set_icon_modulate(0, folder_icon_color);
			}
			filesystem_items[current_path] = item;
		}
		base_path = current_path;
	}

	TreeItem *parent = filesystem_items[base_path];
	return parent;
}

void FindInFilesTree::_draw_result_text(Object *p_item_obj, const Rect2 p_rect) {
	const TreeItem *item = cast_to<TreeItem>(p_item_obj);
	ERR_FAIL_COND_MSG(!item, "Item must be a TreeItem for custom draw.");

	Vector<Variant> ids = item->get_meta("ids", Array());
	List<FindInFilesSearcher::FindResult> results_for_item;
	for (const Variant &id : ids) {
		HashMap<String, FindInFilesSearcher::FindResult>::Iterator found_result = result_id_map.find(id);
		ERR_FAIL_COND_MSG(!found_result, "Result item could not be found for TreeItem id '" + id.stringify() + "'");
		results_for_item.push_back(found_result->value);
	}

	_draw_find_result_tree_item(item, p_rect, &results_for_item);
}

void FindInFilesTree::_draw_find_result_tree_item(const TreeItem *p_item, Rect2 p_rect, List<FindInFilesSearcher::FindResult> *p_results) {
	ERR_FAIL_COND_MSG(p_results->is_empty(), "Results must contain at least one item");

	Ref<Font> font = get_theme_font(SNAME("font"));
	int font_size = get_theme_font_size(SNAME("font_size"));

	FindInFilesSearcher::FindResult first_result = p_results->front()->get();

	int trim_count = p_item->get_meta("trim_count");

	for (const FindInFilesSearcher::FindResult &result : *p_results) {
		int start_highlight_col = -1 * trim_count + result.start_col - (int)p_item->get_meta("display_text_start_column");
		int highlight_length = result.start_line != result.end_line ? -1 : result.end_col - result.start_col;

		Rect2 match_rect = p_rect;
		match_rect.position.x += font->get_string_size(p_item->get_text(0).left(start_highlight_col), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x + get_theme_constant(SNAME("inner_item_margin_left"), SNAME("Tree"));
		match_rect.size.x = font->get_string_size(p_item->get_text(0).substr(start_highlight_col, highlight_length), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
		match_rect.position.y += 1 * EDSCALE;
		match_rect.size.y -= 2 * EDSCALE;

		// Use the inverted accent color to help match rectangles stand out even on the currently selected line.
		draw_rect(match_rect, get_theme_color(SNAME("accent_color"), SNAME("Editor")) * Color(1, 1, 1, 0.35f));
	}

	if (dialog_mode) {
		// Draw file name & line number after result
		// TODO ensure no overlap
	} else {
		// Draw line number after result
		Size2 item_text_size = font->get_string_size(p_item->get_text(0), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
		const String line_text = ":" + itos(first_result.start_line + 1);
		Size2 line_text_size = font->get_string_size(line_text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);

		Point2 info_string_pos = p_rect.get_position() + item_text_size;
		info_string_pos.x += 8; // Buffer between the end of the item text and the start of the info text
		info_string_pos.y += Math::floor((p_rect.size.y - line_text_size.y) * 0.5) - get_theme_constant(SNAME("inner_item_margin_top")); // Center vertically

		draw_string(font, info_string_pos, line_text, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size, secondary_font_color);
	}
}

void FindInFilesTree::_on_result_activated() {
	TreeItem *selected = get_selected();
	if (!selected) {
		return;
	}

	const Variant path = selected->get_metadata(0);
	if (!path.is_string() || ((String)path).is_empty()) {
		selected->set_collapsed(!selected->is_collapsed());
		return;
	}

	if (!FileAccess::exists(path)) {
		return;
	}

	const int line = selected->get_meta("line");
	const int column = selected->get_meta("column");
	emit_signal("open_file_requested", path, line, column);
}

void FindInFilesTree::reset() {
	_remake_empty_tree_structure();

	result_id_map.clear();
	filesystem_items.clear();
}

void FindInFilesTree::_remake_empty_tree_structure() {
	String previous_root_text = "";
	if (get_root()) {
		previous_root_text = get_root()->get_text(0);
	}

	clear();
	create_item();
	get_root()->set_text(0, previous_root_text);
	get_root()->set_icon(0, folder_icon);
}

FindInFilesTree::FindInFilesTree(bool p_dialog_mode) {
	dialog_mode = p_dialog_mode;
	if (dialog_mode) {
		set_columns(2);
	}

	set_hide_root(p_dialog_mode);

	set_h_size_flags(SIZE_EXPAND_FILL);
	set_v_size_flags(SIZE_EXPAND_FILL);

	set_select_mode(SELECT_ROW);
	set_allow_rmb_select(true);

	reset();
}
