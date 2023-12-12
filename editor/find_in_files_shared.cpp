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

void FindInFilesFilePreview::_set_editor(ScriptEditorBase *p_editor) {
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

void FindInFilesFilePreview::clear_file() {
	current_file = "";
	current_line = -1;
	current_file_display->set_text(TTR("Nothing selected"));
	current_file_folder_display->set_text("");
	_set_editor(nullptr);
}

void FindInFilesFilePreview::open_file(const String &p_path, int p_line) {
	if (current_file == p_path && current_line == p_line) {
		return;
	}

	const Ref<Resource> file = ScriptEditor::get_singleton()->open_file(p_path, false);
	if (file.is_valid()) {
		current_file = p_path;
		current_line = p_line;

		current_file_display->set_text(p_path.get_file());
		current_file_folder_display->set_text(p_path.replace(p_path.get_file(), ""));

		_set_editor(ScriptEditor::get_singleton()->create_script_editor(file));
		if (editor) {
			editor->set_edited_resource(file);
			editor->enable_editor();
			editor->goto_line_centered(p_line);
		}
	}
}

void FindInFilesFilePreview::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			current_file_folder_display->add_theme_color_override("font_color", current_file_folder_display->get_theme_color(SNAME("disabled_font_color"), SNAME("Editor")));
		} break;
	}
}

FindInFilesFilePreview::FindInFilesFilePreview() {
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

FindInFilesFilePreview::~FindInFilesFilePreview() {
	clear_file();
	current_file_display->queue_free();
	current_file_folder_display->queue_free();
	editor_panel->queue_free();
}
