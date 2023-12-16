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
#include "find_in_files_dialog.h"
#include "find_in_files_shared.h"
#include "find_in_files_tree.h"
#include "plugins/script_editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"

void FindInFilesPanelTab::_on_result_selected() {
	const TreeItem *selected = results->get_selected();
	if (!selected) {
		replace_button->set_disabled(true);
		file_preview->clear_file();
		return;
	}

	const Array ids = selected->get_meta("ids", Array());
	if (ids.is_empty()) {
		replace_button->set_disabled(true);
		file_preview->clear_file();
		return;
	}

	// All the ids will be for the same file and line, doesnt matter which one we get.
	const String id = ids.front();
	if (result_items.has(id)) {
		const FindInFilesSearcher::FindResult r = result_items[id];
		file_preview->open_file(r.path, r.start_line);
		replace_button->set_disabled(false);
	} else {
		file_preview->clear_file();
		replace_button->set_disabled(true);
	}
}

void FindInFilesPanelTab::_on_dialog_confirmed() {
	FindReplaceConfiguration config;
	FindInFilesSearcher::Status status;
	dialog->get_state(config, status);

	if (config == current_dialog_configuration) {
		return;
	}
	current_dialog_configuration = config;

	searcher->set_configuration(config);

	if (config.is_replace_mode) {
		replace_actions_container->show();
	}

	_run_search();
}

void FindInFilesPanelTab::_on_open_file_requested(const String &p_path, int p_line, int p_column) {
	auto res = ScriptEditor::get_singleton()->open_file(p_path, true);
	ScriptEditor::get_singleton()->edit(res, p_line, p_column);
}

void FindInFilesPanelTab::_do_replace_on_selected() {
	TreeItem *selected = results->get_selected();
	ERR_FAIL_COND_MSG(!selected, "Can't perform replace - nothing selected.");

	const Array ids = selected->get_meta("ids");
	ERR_FAIL_COND_MSG(ids.is_empty(), "Can't perform replace - selected item does not have 'ids' meta");

	// When in replace mode, each item only has a single id in the array.
	const FindInFilesSearcher::FindResult result = result_items.get(ids.front());
	if (searcher->replace_match(result, current_dialog_configuration.replace_text)) {
		_on_result_selected();

		if (!searcher->is_result_valid(result)) {
			results->remove_item(selected);
		}

		ScriptEditor::get_singleton()->reload_scripts();
	}
}

void FindInFilesPanelTab::_update_from_searcher() {
	FindInFilesSearcher::Status status = searcher->get_status();
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

void FindInFilesPanelTab::_soft_limit_continue_search() {
	searcher->release_soft_limit(true);
	update_poll_timer->set_paused(false);
}

void FindInFilesPanelTab::_soft_limit_cancel_search() {
	searcher->release_soft_limit(false);
	update_poll_timer->stop();
	searcher->stop();

	FindInFilesSearcher::Status status = searcher->get_status();
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
	results->reset();
	result_items.clear();
	result_filesystem_levels.clear();

	searcher->start();
	update_poll_timer->start();
}

void FindInFilesPanelTab::expand_collapse_tree(bool p_collapse) {
	ERR_FAIL_COND(!results->get_root());
	results->get_root()->set_collapsed_recursive(p_collapse);
}

void FindInFilesPanelTab::popup_configure() {
	// Cancel current search.
	update_poll_timer->stop();
	searcher->stop();

	dialog->popup_centered_ratio_xy(0.5, 0.65);
}

void FindInFilesPanelTab::rerun_search() {
	_run_search();
}

FindInFilesPanelTab::FindInFilesPanelTab(const FindReplaceConfiguration &p_config, const FindInFilesSearcher::Status &p_status) {
	// TODO Soft Result Limit
	constexpr int result_limit = 1000;
	searcher = memnew(FindInFilesSearcher(result_limit, true));
	searcher->set_configuration(p_config);

	dialog = memnew(FindInFilesDialog2);
	add_child(dialog);
	dialog->set_state(p_config, p_status);
	dialog->set_run_search_on_popup(false);
	dialog->connect(SNAME("confirmed"), callable_mp(this, &FindInFilesPanelTab::_on_dialog_confirmed));

	update_poll_timer = memnew(Timer);
	update_poll_timer->set_wait_time(0.05);
	update_poll_timer->connect(SNAME("timeout"), callable_mp(this, &FindInFilesPanelTab::_update_from_searcher));
	add_child(update_poll_timer);

	// The results list & editor
	HSplitContainer *split = memnew(HSplitContainer);
	split->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	split->set_v_size_flags(SIZE_EXPAND_FILL);
	split->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(split);

	VBoxContainer *left_container = memnew(VBoxContainer);
	left_container->set_h_size_flags(SIZE_EXPAND_FILL);
	left_container->set_v_size_flags(SIZE_EXPAND_FILL);
	left_container->set_stretch_ratio(0.5);
	split->add_child(left_container);

	results = memnew(FindInFilesTree(false));
	results->set_group_results_on_same_line(true);
	results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesPanelTab::_on_result_selected));
	// results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesPanelTab::_update_replace_preview));
	results->connect(SNAME("open_file_requested"), callable_mp(this, &FindInFilesPanelTab::_on_open_file_requested));
	left_container->add_child(results);

	replace_actions_container = memnew(HBoxContainer);
	replace_actions_container->set_visible(p_config.is_replace_mode);
	left_container->add_child(replace_actions_container);

	replace_button = memnew(Button);
	replace_button->set_text("Replace");
	replace_button->set_disabled(true);
	replace_button->connect(SNAME("pressed"), callable_mp(this, &FindInFilesPanelTab::_do_replace_on_selected));
	replace_actions_container->add_child(replace_button);

	Button *replace_all_button = memnew(Button);
	replace_all_button->set_text("Replace All");
	replace_actions_container->add_child(replace_all_button);

	file_preview = memnew(FindInFilesFilePreview);
	split->add_child(file_preview);

	continue_confirm_dialog = memnew(ConfirmationDialog);
	add_child(continue_confirm_dialog);
	continue_confirm_dialog->connect("confirmed", callable_mp(this, &FindInFilesPanelTab::_soft_limit_continue_search));
	continue_confirm_dialog->connect("canceled", callable_mp(this, &FindInFilesPanelTab::_soft_limit_cancel_search));
	continue_confirm_dialog->set_text(vformat(TTR("%s+ results have been found. Do you wish to continue the search?\nThis may take a long time. Editor performance may degrade."), result_limit));
}

FindInFilesPanelTab::~FindInFilesPanelTab() {
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

	// Hide the bottom panel tab when the last search is closed
	if (tabs->get_tab_count() == 0) {
		emit_signal("tabs_empty");
	}
}

void FindInFilesPanel2::_on_rerun_pressed() {
	FindInFilesPanelTab *tab = cast_to<FindInFilesPanelTab>(tabs->get_current_tab_control());
	if (!tab) {
		return;
	}

	tab->rerun_search();
}

void FindInFilesPanel2::_on_configure_pressed() {
	FindInFilesPanelTab *tab = cast_to<FindInFilesPanelTab>(tabs->get_current_tab_control());
	if (!tab) {
		return;
	}

	tab->popup_configure();
}

void FindInFilesPanel2::_on_configure_cancelled() {
	configuring_tab = nullptr;
}

void FindInFilesPanel2::add_tab(FindInFilesDialog2 *p_dialog) {
	if (!p_dialog) {
		return;
	}

	FindReplaceConfiguration config;
	FindInFilesSearcher::Status status;
	p_dialog->get_state(config, status);

	FindInFilesPanelTab *child = memnew(FindInFilesPanelTab(config, status));

	const String mode = config.is_replace_mode ? "Replace" : "Find";
	const String replacement_suffix = config.is_replace_mode ? vformat(" with '%s'", config.replace_text) : "";

	tabs->add_child(child);
	tabs->set_tab_title(tabs->get_tab_count() - 1, vformat("%s '%s'%s", mode, config.text, replacement_suffix));
	tabs->set_tab_button_icon(tabs->get_tab_count() - 1, get_editor_theme_icon(SNAME("Close")));
	tabs->set_current_tab(tabs->get_tab_idx_from_control(child));

	emit_signal("tab_added");
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

			for (int i = 0; i < tabs->get_tab_count(); ++i) {
				tabs->set_tab_button_icon(i, get_editor_theme_icon(SNAME("Close")));
			}
		} break;
	}
}

void FindInFilesPanel2::_bind_methods() {
	ADD_SIGNAL(MethodInfo("tabs_empty"));
	ADD_SIGNAL(MethodInfo("tab_added"));
}

FindInFilesPanel2::FindInFilesPanel2() {
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
	refresh_btn->connect(SNAME("pressed"), callable_mp(this, &FindInFilesPanel2::_on_rerun_pressed));
	buttons_vbox->add_child(refresh_btn);

	configure_btn = memnew(Button);
	configure_btn->set_tooltip_text(TTR("Configure"));
	configure_btn->set_flat(true);
	configure_btn->connect(SNAME("pressed"), callable_mp(this, &FindInFilesPanel2::_on_configure_pressed));
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

	tabs = memnew(TabContainer);
	tabs->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	tabs->set_v_size_flags(SIZE_EXPAND_FILL);
	tabs->set_h_size_flags(SIZE_EXPAND_FILL);
	tabs->connect(SNAME("tab_button_pressed"), callable_mp(this, &FindInFilesPanel2::_on_tab_closed));
	tabs->connect(SNAME("tab_changed"), callable_mp(this, &FindInFilesPanel2::_on_tab_changed));

	main_hbox->add_child(tabs);
}
