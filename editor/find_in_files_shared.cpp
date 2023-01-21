/*************************************************************************/
/*  find_in_files_shared.cpp                                             */
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

#include "find_in_files_shared.h"

#include "editor_scale.h"
#include "plugins/script_editor_plugin.h"
#include "scene/gui/tree.h"
#include "scene/resources/font.h"

void FindInFilesEditor::_set_editor(ScriptEditorBase *p_editor) {
	if (editor_panel->get_child_count(false) == 1) {
		Node *node = editor_panel->get_child(0, false);
		editor_panel->remove_child(node);
		node->queue_free();
	}

	editor = p_editor;
	if (editor) {
		editor_panel->add_child(editor);
		editor->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		// Read only editing for now. Live editing in the dialog is in the 'too hard' basket.
		Control *base_editor = editor->get_base_editor();
		if (base_editor->has_method("set_editable")) {
			base_editor->call("set_editable", false);
		}
	}
}

void FindInFilesEditor::clear_file() {
	current_file_display->set_text(TTR("No file selected."));
	current_file_folder_display->set_text("");
	_set_editor(nullptr);
}

void FindInFilesEditor::open_file(String p_path, int p_line) {
	current_file_display->set_text(p_path.get_file());
	current_file_folder_display->set_text(p_path.replace(p_path.get_file(), ""));

	const Ref<Resource> file = ScriptEditor::get_singleton()->open_file(p_path, false);
	if (file.is_valid()) {
		_set_editor(ScriptEditor::get_singleton()->create_script_editor(file));
		if (editor) {
			editor->set_edited_resource(file);
			editor->enable_editor();
			editor->goto_line_centered(p_line);
		}
	}
}

void FindInFilesEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			current_file_folder_display->add_theme_color_override("font_color", current_file_folder_display->get_theme_color(SNAME("disabled_font_color"), SNAME("Editor")));
		} break;
	}
}

FindInFilesEditor::FindInFilesEditor() {
	set_v_size_flags(Control::SIZE_EXPAND_FILL);
	set_h_size_flags(Control::SIZE_EXPAND_FILL);

	HBoxContainer *file_display_hbox = memnew(HBoxContainer);
	add_child(file_display_hbox);

	current_file_display = memnew(Label);
	file_display_hbox->add_child(current_file_display);

	current_file_folder_display = memnew(Label);
	file_display_hbox->add_child(current_file_folder_display);

	editor_panel = memnew(Panel);
	editor_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(editor_panel);
}

FindInFilesEditor::~FindInFilesEditor() {
	clear_file();
	current_file_display->queue_free();
	current_file_folder_display->queue_free();
	editor_panel->queue_free();
}

void draw_find_result_tree_item(Tree *p_tree, const TreeItem *p_item, Rect2 p_rect, FindInFilesSearcher::FindResult p_result, bool p_draw_line_only) {
	Ref<Font> font = p_tree->get_theme_font(SNAME("font"));
	int font_size = p_tree->get_theme_font_size(SNAME("font_size"));

	int original_size = p_result.line_begin_string.size();
	int trimmed_size = p_item->get_text(0).size();

	if (trimmed_size < 150) {
		int start_highlight_col = trimmed_size - original_size + p_result.start_col;
		int highlight_length = p_result.start_line != p_result.end_line ? -1 : p_result.end_col - p_result.start_col;

		Rect2 match_rect = p_rect;
		match_rect.position.x += font->get_string_size(p_item->get_text(0).left(start_highlight_col), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
		match_rect.size.x = font->get_string_size(p_item->get_text(0).substr(start_highlight_col, highlight_length), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
		match_rect.position.y += 1 * EDSCALE;
		match_rect.size.y -= 2 * EDSCALE;

		// Use the inverted accent color to help match rectangles stand out even on the currently selected line.
		p_tree->draw_rect(match_rect, p_tree->get_theme_color(SNAME("accent_color"), SNAME("Editor")).inverted() * Color(1, 1, 1, 0.35f));
	}

	Point2 info_string_pos;
	if (p_draw_line_only) {
		info_string_pos = font->get_string_size(p_item->get_text(0), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size) + p_rect.get_position() + Point2(5, 0);
	} else {
		info_string_pos = Point2(p_rect.get_end().x, p_rect.get_position().y);
	}

	const String file_text = p_result.path.get_file();
	const String line_text = ":" + itos(p_result.start_line + 1);
	const String full_text = p_draw_line_only ? line_text : file_text + line_text;

	if (!p_draw_line_only) {
		const Size2 full_string_size = font->get_string_size(full_text, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size);
		info_string_pos.x -= 2 * EDSCALE + full_string_size.width;
		info_string_pos.y += p_rect.size.y - full_string_size.y / 2;
	}

	p_tree->draw_string(font, info_string_pos, full_text, HORIZONTAL_ALIGNMENT_RIGHT, -1, font_size, Color(1, 1, 1, 0.4f));
}
