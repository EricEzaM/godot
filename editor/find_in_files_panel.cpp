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

#include "editor/find_in_files_panel.h"

#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "find_in_files_shared.h"
#include "find_in_files_tree.h"
#include "plugins/script_editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"

FindInFilesPanel2 *FindInFilesPanel2::singleton = nullptr;

void FindInFilesPanelTab::_on_open_file_requested(const String &p_path, int p_line, int p_column) {
	auto res = ScriptEditor::get_singleton()->open_file(p_path, true);
	ScriptEditor::get_singleton()->edit(res, p_line, p_column);
}

void FindInFilesPanelTab::_reset() {
	// TODO results tree .reset()

	result_items.clear();
	result_filesystem_levels.clear();
}

void FindInFilesPanelTab::_update_search_status() {
	FindInFilesSearcher::FindInFilesStatus status = searcher->get_status();
	results->get_root()->set_text(0, vformat(TTR("%s results"), status.results.size()));

	for (const FindInFilesSearcher::FindResult &result : status.results) {
		results->add_result(result);
		result_items[result.id] = result;
	}

	// Select first result when it is available - do not override user selection.
	if (!results->get_selected()) {
		results->select_first_non_root();
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

void FindInFilesPanelTab::_update_file_preview() {
	const TreeItem *selected = results->get_selected();
	if (!selected) {
		file_preview->clear_file();
		return;
	}

	const Array ids = selected->get_meta("ids", Array());
	if (ids.is_empty()) {
		file_preview->clear_file();
		return;
	}

	// All the ids will be for the same file, doesnt matter which one we get.
	const String id = ids.front();
	if (result_items.has(id)) {
		const FindInFilesSearcher::FindResult r = result_items[id];
		file_preview->open_file(r.path, r.start_line);
	} else {
		file_preview->clear_file();
	}
}

void FindInFilesPanelTab::_soft_limit_continue_search() {
	searcher->release_soft_limit(true);
	update_poll_timer->set_paused(false);
}

void FindInFilesPanelTab::_soft_limit_cancel_search() {
	searcher->release_soft_limit(false);
	update_poll_timer->stop();
	searcher->stop();

	FindInFilesSearcher::FindInFilesStatus status = searcher->get_status();
	results->get_root()->set_text(0, vformat(TTR("%s+ results (search was stopped early)"), status.results.size()));
}

void FindInFilesPanelTab::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			file_icon = get_editor_theme_icon(SNAME("File"));
			folder_icon = get_editor_theme_icon(SNAME("Folder"));
			folder_icon_color = get_theme_color(SNAME("folder_icon_color"), SNAME("FileDialog"));
		} break;
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
	_reset();

	searcher->start();
	update_poll_timer->start();
}

void FindInFilesPanelTab::expand_collapse_tree(bool p_collapse) {
	ERR_FAIL_COND(!results->get_root());
	results->get_root()->set_collapsed_recursive(p_collapse);
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

	results = memnew(FindInFilesTree(false));
	results->set_stretch_ratio(0.5);
	results->set_group_results_on_same_line(true);
	results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesPanelTab::_update_file_preview));
	// results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesPanelTab::_update_replace_preview));
	results->connect(SNAME("open_file_requested"), callable_mp(this, &FindInFilesPanelTab::_on_open_file_requested));
	split->add_child(results);

	file_preview = memnew(FindInFilesFilePreview);
	split->add_child(file_preview);

	continue_confirm_dialog = memnew(ConfirmationDialog);
	add_child(continue_confirm_dialog);
	continue_confirm_dialog->connect("confirmed", callable_mp(this, &FindInFilesPanelTab::_soft_limit_continue_search));
	continue_confirm_dialog->connect("canceled", callable_mp(this, &FindInFilesPanelTab::_soft_limit_cancel_search));
	continue_confirm_dialog->set_text(vformat(TTR("%s+ results have been found. Do you wish to continue the search?\nThis may take a long time. Editor performance may degrade."), result_limit));
}

FindInFilesPanelTab::~FindInFilesPanelTab() {
	file_preview->queue_free();
	continue_confirm_dialog->queue_free();
	// status_display->queue_free();
	results->queue_free();
	update_poll_timer->queue_free();
	memdelete(searcher);
}

void FindInFilesPanel2::_on_tab_changed(int p_new_tab) {
	Control *current = tabs->get_tab_control(p_new_tab);
	if (!current) {
		return;
	}
	FindInFilesPanelTab *tab = cast_to<FindInFilesPanelTab>(current);
	ERR_FAIL_NULL_MSG(tab, "Tab of find in files panel is of incorrect type");
}

void FindInFilesPanel2::_on_tab_closed(int p_tab) {
	Control *current = tabs->get_tab_control(p_tab);
	if (!current) {
		return;
	}

	FindInFilesPanelTab *tab = cast_to<FindInFilesPanelTab>(current);
	ERR_FAIL_NULL_MSG(tab, "Tab of find in files panel is of incorrect type");

	tabs->remove_child(tab);
	tab->queue_free();

	// Remove the bottom panel tab when the last search is closed
	if (tabs->get_tab_count() == 0 && has_editor_panel) {
		EditorNode::get_singleton()->remove_bottom_panel_item(this);
		has_editor_panel = false;
	}
}

void FindInFilesPanel2::_expand_collapse_tree(bool p_collapse) {
	Control *current = tabs->get_current_tab_control();
	if (!current) {
		return;
	}
	FindInFilesPanelTab *tab = cast_to<FindInFilesPanelTab>(current);
	ERR_FAIL_NULL_MSG(tab, "Tab of find in files panel is of incorrect type");

	tab->expand_collapse_tree(p_collapse);
}

void FindInFilesPanel2::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			refresh_btn->set_icon(get_editor_theme_icon(SNAME("Reload")));
			configure_btn->set_icon(get_editor_theme_icon(SNAME("Tools")));
			expand_all_btn->set_icon(get_editor_theme_icon(SNAME("ExpandTree")));
			collapse_all_btn->set_icon(get_editor_theme_icon(SNAME("CollapseTree")));
			show_source_btn->set_icon(get_editor_theme_icon(SNAME("Script")));

			for (int i = 0; i < tabs->get_tab_count(); ++i) {
				tabs->set_tab_button_icon(i, get_editor_theme_icon(SNAME("Close")));
			}
		} break;
	}
}

void FindInFilesPanel2::add_search(FindInFilesSearcher::SearchInputData p_input_data) {
	if (!has_editor_panel) {
		EditorNode::get_singleton()->add_bottom_panel_item(TTR("Search Results"), get_singleton());
		has_editor_panel = true;
	}

	EditorNode::get_singleton()->make_bottom_panel_item_visible(get_singleton());

	FindInFilesPanelTab *child = memnew(FindInFilesPanelTab(p_input_data));
	tabs->add_child(child);
	tabs->set_tab_title(tabs->get_tab_count() - 1, vformat("Find '%s'", p_input_data.text));
	tabs->set_tab_button_icon(tabs->get_tab_count() - 1, get_editor_theme_icon(SNAME("Close")));
	tabs->set_current_tab(tabs->get_tab_idx_from_control(child));
}

FindInFilesPanel2::FindInFilesPanel2() {
	ERR_FAIL_COND(singleton != nullptr);
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

	expand_all_btn = memnew(Button);
	expand_all_btn->set_tooltip_text(TTR("Expand All"));
	expand_all_btn->set_flat(true);
	expand_all_btn->connect(SNAME("pressed"), callable_mp(this, &FindInFilesPanel2::_expand_collapse_tree).bind(false));
	buttons_vbox->add_child(expand_all_btn);

	collapse_all_btn = memnew(Button);
	collapse_all_btn->set_tooltip_text(TTR("Collapse All"));
	collapse_all_btn->set_flat(true);
	collapse_all_btn->connect(SNAME("pressed"), callable_mp(this, &FindInFilesPanel2::_expand_collapse_tree).bind(true));
	buttons_vbox->add_child(collapse_all_btn);

	show_source_btn = memnew(Button);
	show_source_btn->set_tooltip_text(TTR("Show source preview"));
	show_source_btn->set_flat(true);
	buttons_vbox->add_child(show_source_btn);

	tabs = memnew(TabContainer);
	tabs->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	tabs->set_v_size_flags(SIZE_EXPAND_FILL);
	tabs->set_h_size_flags(SIZE_EXPAND_FILL);
	tabs->connect(SNAME("tab_button_pressed"), callable_mp(this, &FindInFilesPanel2::_on_tab_closed));
	tabs->connect(SNAME("tab_changed"), callable_mp(this, &FindInFilesPanel2::_on_tab_changed));

	main_hbox->add_child(tabs);
}
