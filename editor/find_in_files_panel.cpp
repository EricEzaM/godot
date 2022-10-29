/*************************************************************************/
/*  find_in_files_panel.cpp                                              */
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

#include "find_in_files_panel.h"

#include "editor_node.h"
#include "scene/gui/box_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"

FindInFilesPanelTab::FindInFilesPanelTab() {
	// The results list & editor
	HSplitContainer *split = memnew(HSplitContainer);
	split->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	split->set_v_size_flags(SIZE_EXPAND_FILL);
	split->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(split);

	results = memnew(Tree);
	results->set_stretch_ratio(0.5);
	results->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	results->create_item(); // Root
	split->add_child(results);

	editor_container = memnew(PanelContainer);
	editor_container->set_h_size_flags(SIZE_EXPAND_FILL);
	split->add_child(editor_container);
}

void FindInFilesPanel2::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			refresh_btn->set_icon(get_theme_icon(SNAME("Reload"), SNAME("EditorIcons")));
			grouping_btn->set_icon(get_theme_icon(SNAME("Groups"), SNAME("EditorIcons")));
			expand_all_btn->set_icon(get_theme_icon(SNAME("ExpandTree"), SNAME("EditorIcons")));
			collapse_all_btn->set_icon(get_theme_icon(SNAME("CollapseTree"), SNAME("EditorIcons")));
			show_source_btn->set_icon(get_theme_icon(SNAME("Script"), SNAME("EditorIcons")));
		} break;
	}
}

FindInFilesPanel2::FindInFilesPanel2() {
	HBoxContainer *main_hbox = memnew(HBoxContainer);
	main_hbox->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	main_hbox->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(main_hbox);

	// Buttons
	VBoxContainer *buttons_vbox = memnew(VBoxContainer);
	main_hbox->add_child(buttons_vbox);

	refresh_btn = memnew(Button);
	refresh_btn->set_tooltip_text(TTR("Rerun"));
	refresh_btn->set_flat(true);
	buttons_vbox->add_child(refresh_btn);

	grouping_btn = memnew(MenuButton);
	grouping_btn->set_tooltip_text(TTR("Grouping Options"));
	grouping_btn->set_flat(true);
	buttons_vbox->add_child(grouping_btn);

	expand_all_btn = memnew(Button);
	expand_all_btn->set_tooltip_text(TTR("Expand All"));
	expand_all_btn->set_flat(true);
	buttons_vbox->add_child(expand_all_btn);

	collapse_all_btn = memnew(Button);
	collapse_all_btn->set_tooltip_text(TTR("Collapse All"));
	collapse_all_btn->set_flat(true);
	buttons_vbox->add_child(collapse_all_btn);

	show_source_btn = memnew(Button);
	show_source_btn->set_tooltip_text(TTR("Show source preview"));
	show_source_btn->set_flat(true);
	buttons_vbox->add_child(show_source_btn);

	Button *btn = EditorNode::get_singleton()->add_bottom_panel_item(TTR("Search Results2"), this);

	tabs = memnew(TabContainer);
	tabs->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	tabs->set_v_size_flags(SIZE_EXPAND_FILL);
	tabs->set_h_size_flags(SIZE_EXPAND_FILL);
	auto child = memnew(FindInFilesPanelTab);
	tabs->add_child(child);
	tabs->set_tab_title(0, "Find: testing");

	main_hbox->add_child(tabs);
}
