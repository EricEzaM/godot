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
#include "editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"

FindInFilesPanel2 *FindInFilesPanel2::singleton = nullptr;

// TODO can this be shared between dialog and parent?
void FindInFilesPanelTab::_update_search_status() {
	// TODO ensure this can only be entered once. Do an if(processing) or something
	// Also do it for the dialog

	auto status = searcher->get_status();
	// status_display->set_text(vformat("%s%s matches in %s%s files", status.results.size(), status.limit_reached ? "+" : "", status.files_with_matches, status.limit_reached ? "+" : ""));

	if (status.results.size() == 0) {
		// _update_mini_editor();
	}

	results->get_root()->set_text(0, vformat("%s results", status.results.size()));
	for (const FindInFilesSearcher::FindResult &r : status.results) {
		String result_id = vformat("%s_%s_%s_%s_%s", r.path, r.start_line, r.start_col, r.end_line, r.end_col);
		if (result_items.has(result_id)) {
			continue;
		}

		// TODO Cleanup, it's a bit gross
		String path_no_res = r.path.substr(6);
		String combined = "";
		int slice_count = path_no_res.get_slice_count("/");
		for (int i = 0; i < slice_count; ++i) {
			String slice = path_no_res.get_slicec('/', i);
			if (!result_filesystem_levels.has(combined + slice)) {
				TreeItem *item = result_filesystem_levels.has(combined) ? results->create_item(result_filesystem_levels[combined]) : results->create_item();
				item->set_text(0, slice);
				item->set_cell_mode(0, TreeItem::CELL_MODE_STRING);
				item->set_icon(0, FileAccess::exists("res://" + combined + slice) ? get_theme_icon(SNAME("File"), SNAME("EditorIcons")) : get_theme_icon(SNAME("Folder"), SNAME("EditorIcons")));
				result_filesystem_levels[combined + slice] = item;
			}
			combined += slice + (i != slice_count - 1 ? "/" : "");
		}

		TreeItem *parent = result_filesystem_levels[combined];
		TreeItem *item = results->create_item(parent);
		item->set_cell_mode(0, TreeItem::CELL_MODE_STRING);

		String text = r.line_begin_string;
		text = text.strip_edges(true, false);
		// Only draw text up to a limit to prevent slowdown due to long one-liner files.
		if (text.size() > 150) {
			text = text.substr(0, 150) + "...";
		}

		item->set_text(0, text);
		item->set_metadata(0, r.path);
		item->set_meta("id", result_id);

		// TODO combined results which happen on the same line into one tree item.
		result_items[result_id] = r;
	}

	// Select first result when it is available - do not override user selection.
	if (!results->get_selected() && results->get_root() && results->get_root()->get_first_child()) {
		results->get_root()->get_first_child()->select(0);
	}

	if (status.limit_reached && status.soft_limit) {
		update_poll_timer->set_paused(true);
		continue_confirm_dialog->popup_centered();
	}
	if (status.finished) {
		update_poll_timer->stop();
	}
}

void FindInFilesPanelTab::_update_searcher(FindInFilesSearcher::SearchInputData p_input_data) {
	searcher->set_search_text(p_input_data.text);
	searcher->set_directory(p_input_data.directory);
	searcher->set_file_filter(p_input_data.allow_file_regex_strings, p_input_data.ignore_file_regex_strings);
	searcher->set_case_sensitive(p_input_data.match_case_sensitive);
	searcher->set_whole_words(p_input_data.match_whole_words);
	searcher->set_use_regex(p_input_data.match_use_regex);
}

void FindInFilesPanelTab::_soft_limit_continue_search() {
	searcher->release_soft_limit(true);
	update_poll_timer->set_paused(false);
}

void FindInFilesPanelTab::_soft_limit_cancel_search() {
	searcher->release_soft_limit(false);
	update_poll_timer->stop();
	searcher->stop();
}

void FindInFilesPanelTab::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			if (update_poll_timer->is_stopped() && !searcher->get_status().finished) {
				_run_search();
			}
		} break;
	}
}

void FindInFilesPanelTab::_run_search() {
	// Cancel current search.
	update_poll_timer->stop();
	searcher->stop();

	// Clear results
	result_items.clear();
	results->clear();
	TreeItem *root = results->create_item();
	root->set_text(0, "res://");
	root->set_icon(0, get_theme_icon(SNAME("Folder"), SNAME("EditorIcons")));

	searcher->start();
	update_poll_timer->start();
}

FindInFilesPanelTab::FindInFilesPanelTab(FindInFilesSearcher::SearchInputData p_input_data) {
	// TODO Soft Result Limit
	const int result_limit = 1000;
	searcher = memnew(FindInFilesSearcher);
	searcher->set_result_limit(result_limit, true);
	_update_searcher(p_input_data);

	update_poll_timer = memnew(Timer);
	update_poll_timer->set_wait_time(0.05);
	update_poll_timer->connect(SNAME("timeout"), callable_mp(this, &FindInFilesPanelTab::_update_search_status));
	add_child(update_poll_timer);

	// The results list & editor
	HSplitContainer *split = memnew(HSplitContainer);
	split->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	split->set_v_size_flags(SIZE_EXPAND_FILL);
	split->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(split);

	results = memnew(Tree);
	results->set_stretch_ratio(0.5);
	results->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	results->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	// results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesDialog2::_update_mini_editor));
	// results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesDialog2::_update_replace_preview));
	// results->connect(SNAME("item_activated"), callable_mp(this, &FindInFilesDialog2::_on_result_activated));
	results->set_select_mode(Tree::SELECT_ROW);
	results->set_allow_rmb_select(true);
	split->add_child(results);

	editor_container = memnew(PanelContainer);
	editor_container->set_h_size_flags(SIZE_EXPAND_FILL);
	split->add_child(editor_container);

	continue_confirm_dialog = memnew(ConfirmationDialog);
	add_child(continue_confirm_dialog);
	continue_confirm_dialog->connect("confirmed", callable_mp(this, &FindInFilesPanelTab::_soft_limit_continue_search));
	continue_confirm_dialog->connect("cancelled", callable_mp(this, &FindInFilesPanelTab::_soft_limit_cancel_search));
	continue_confirm_dialog->set_text(TTR(vformat("%s+ results have been found. Do you wish to continue the search? This may take a long time.", result_limit)));
}

void FindInFilesPanel2::_on_tab_button_pressed(int p_tab) {
	// TODO close tab
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

			for (int i = 0; i < tabs->get_tab_count(); ++i) {
				tabs->set_tab_button_icon(i, get_theme_icon(SNAME("Close"), SNAME("EditorIcons")));
			}
		} break;
	}
}

void FindInFilesPanel2::add_search(FindInFilesSearcher::SearchInputData p_input_data) {
	FindInFilesPanelTab *child = memnew(FindInFilesPanelTab(p_input_data));
	tabs->add_child(child);
	tabs->set_tab_title(tabs->get_tab_count() - 1, vformat("Find '%s'", p_input_data.text));
	tabs->set_tab_button_icon(tabs->get_tab_count() - 1, get_theme_icon(SNAME("Close"), SNAME("EditorIcons")));
}

FindInFilesPanel2::FindInFilesPanel2() {
	singleton = this;

	set_custom_minimum_size(Size2(0, 200) * EDSCALE);

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

	configure_btn = memnew(Button);
	configure_btn->set_tooltip_text(TTR("Configure"));
	configure_btn->set_flat(true);
	buttons_vbox->add_child(configure_btn);

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

	tabs = memnew(TabContainer);
	tabs->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	tabs->set_v_size_flags(SIZE_EXPAND_FILL);
	tabs->set_h_size_flags(SIZE_EXPAND_FILL);
	tabs->connect(SNAME("tab_button_pressed"), callable_mp(this, &FindInFilesPanel2::_on_tab_button_pressed));

	main_hbox->add_child(tabs);
}
