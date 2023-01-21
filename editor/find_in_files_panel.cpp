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
#include "plugins/script_editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"

FindInFilesPanel2 *FindInFilesPanel2::singleton = nullptr;

void FindInFilesPanelTab::_on_result_activated() {
	TreeItem *selected = results->get_selected();
	if (!selected) {
		return;
	}

	String path = selected->get_metadata(0);
	if (path.is_empty()) {
		return;
	}

	ScriptEditor::get_singleton()->open_file(path, true);
	hide();
}

void FindInFilesPanelTab::_reset() {
	results->clear();
	results->create_item();
	results->get_root()->set_text(0, "res://");
	results->get_root()->set_icon(0, folder_icon);

	result_items.clear();
	result_filesystem_levels.clear();
}

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

		TreeItem *parent;
		bool is_group_dir = (grouping_mode & DIRECTORY) == DIRECTORY;
		bool is_group_file = (grouping_mode & FILE) == FILE;
		if (!is_group_dir && !is_group_file) {
			parent = results->get_root();
		} else if (is_group_file && !is_group_dir) {
			String file_name = r.path.get_file();
			if (!result_filesystem_levels.has(file_name)) {
				TreeItem *item = results->create_item();
				item->set_text(0, file_name);
				item->set_tooltip_text(0, r.path);
				item->set_icon(0, file_icon);
				result_filesystem_levels[file_name] = item;
			}

			parent = result_filesystem_levels[file_name];
		} else {
			String path_no_res = r.path.substr(6);
			int slice_count = path_no_res.get_slice_count("/");
			String base_path = "";

			for (int i = 0; i < slice_count; ++i) {
				if (i == slice_count - 1 && !is_group_file) {
					continue;
				}

				String slice = path_no_res.get_slicec('/', i);
				String current_path = base_path + (i == 0 ? "" : "/") + slice;
				if (!result_filesystem_levels.has(current_path)) {
					TreeItem *item = result_filesystem_levels.has(base_path) ? results->create_item(result_filesystem_levels[base_path]) : results->create_item();
					item->set_text(0, slice);
					if (i == slice_count - 1) {
						item->set_icon(0, file_icon);
						item->set_tooltip_text(0, r.path);
					} else {
						item->set_icon(0, folder_icon);
					}
					result_filesystem_levels[current_path] = item;
				}
				base_path = current_path;
			}

			parent = result_filesystem_levels[base_path];
		}

		TreeItem *item = results->create_item(parent);
		item->set_cell_mode(0, TreeItem::CELL_MODE_CUSTOM);

		String text = r.line_begin_string;
		text = text.strip_edges(true, false);
		// Only draw text up to a limit to prevent slowdown due to long one-liner files.
		if (text.size() > 150) {
			text = text.substr(0, 150) + "...";
		}

		item->set_text(0, text);
		item->set_metadata(0, r.path);
		item->set_meta("id", result_id);
		item->set_custom_draw(0, this, "_draw_result_text");

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

void FindInFilesPanelTab::_update_editor() {
	const TreeItem *selected = results->get_selected();
	if (!selected) {
		editor_panel->clear_file();
		return;
	}

	const String id = selected->get_meta("id", "");
	if (id.is_empty()) {
		editor_panel->clear_file();
		return;
	}

	if (result_items.has(id)) {
		const FindInFilesSearcher::FindResult r = result_items[id];
		editor_panel->open_file(r.path, r.start_line);
	} else {
		editor_panel->clear_file();
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
}

void FindInFilesPanelTab::_draw_result_text(Object *p_item_obj, const Rect2 p_rect) {
	TreeItem *item = Object::cast_to<TreeItem>(p_item_obj);
	ERR_FAIL_COND_MSG(!item, "Item must be a TreeItem for custom draw.");

	const String id = item->get_meta("id", "");
	HashMap<String, FindInFilesSearcher::FindResult>::Iterator E = result_items.find(id);
	ERR_FAIL_COND_MSG(!E, "Result item could not be found for TreeItem id '" + id + "'");

	bool is_group_file = (grouping_mode & FILE) == FILE;
	draw_find_result_tree_item(results, item, p_rect, E->value, is_group_file);
}

void FindInFilesPanelTab::_bind_methods() {
	ClassDB::bind_method("_draw_result_text", &FindInFilesPanelTab::_draw_result_text);

	BIND_ENUM_CONSTANT(GroupingModeFlags::FILE)
	BIND_ENUM_CONSTANT(GroupingModeFlags::DIRECTORY)
}

void FindInFilesPanelTab::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			file_icon = get_theme_icon(SNAME("File"), SNAME("EditorIcons"));
			folder_icon = get_theme_icon(SNAME("Folder"), SNAME("EditorIcons"));
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

void FindInFilesPanelTab::set_grouping_mode(int p_mode) {
	grouping_mode = p_mode;
	_reset();
	_update_search_status();
}

int FindInFilesPanelTab::get_grouping_mode() const {
	return grouping_mode;
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
	results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesPanelTab::_update_editor));
	// results->connect(SNAME("item_selected"), callable_mp(this, &FindInFilesPanelTab::_update_replace_preview));
	results->connect(SNAME("item_activated"), callable_mp(this, &FindInFilesPanelTab::_on_result_activated));
	results->set_select_mode(Tree::SELECT_ROW);
	results->set_allow_rmb_select(true);
	split->add_child(results);

	editor_panel = memnew(FindInFilesEditor);
	split->add_child(editor_panel);

	continue_confirm_dialog = memnew(ConfirmationDialog);
	add_child(continue_confirm_dialog);
	continue_confirm_dialog->connect("confirmed", callable_mp(this, &FindInFilesPanelTab::_soft_limit_continue_search));
	continue_confirm_dialog->connect("cancelled", callable_mp(this, &FindInFilesPanelTab::_soft_limit_cancel_search));
	continue_confirm_dialog->set_text(TTR(vformat("%s+ results have been found. Do you wish to continue the search? This may take a long time.", result_limit)));
}

FindInFilesPanelTab::~FindInFilesPanelTab() {
	editor_panel->queue_free();
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

	const int mode = tab->get_grouping_mode();
	group_files_btn->set_pressed((mode & FindInFilesPanelTab::GroupingModeFlags::FILE) == FindInFilesPanelTab::GroupingModeFlags::FILE);
	group_directory_btn->set_pressed((mode & FindInFilesPanelTab::GroupingModeFlags::DIRECTORY) == FindInFilesPanelTab::GroupingModeFlags::DIRECTORY);
}

void FindInFilesPanel2::_on_tab_button_pressed(int p_tab) {
	Control *current = tabs->get_tab_control(p_tab);
	if (!current) {
		return;
	}
	FindInFilesPanelTab *tab = cast_to<FindInFilesPanelTab>(current);
	ERR_FAIL_NULL_MSG(tab, "Tab of find in files panel is of incorrect type");

	tabs->remove_child(tab);
	tab->queue_free();
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

void FindInFilesPanel2::_toggle_grouping(bool p_toggled_on, FindInFilesPanelTab::GroupingModeFlags flag) {
	Control *current = tabs->get_current_tab_control();
	FindInFilesPanelTab *tab = cast_to<FindInFilesPanelTab>(current);
	ERR_FAIL_NULL_MSG(tab, "Tab of find in files panel is of incorrect type");

	int mode = tab->get_grouping_mode();
	if (p_toggled_on) {
		mode |= flag;
	} else {
		mode &= ~flag;
	}

	tab->set_grouping_mode(mode);
}

void FindInFilesPanel2::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
		case NOTIFICATION_THEME_CHANGED: {
			refresh_btn->set_icon(get_theme_icon(SNAME("Reload"), SNAME("EditorIcons")));
			configure_btn->set_icon(get_theme_icon(SNAME("Tools"), SNAME("EditorIcons")));
			group_directory_btn->set_icon(get_theme_icon(SNAME("Folder"), SNAME("EditorIcons")));
			group_files_btn->set_icon(get_theme_icon(SNAME("File"), SNAME("EditorIcons")));
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

	group_directory_btn = memnew(Button);
	group_directory_btn->set_tooltip_text(TTR("Group by directory"));
	group_directory_btn->set_flat(true);
	group_directory_btn->set_toggle_mode(true);
	group_directory_btn->set_pressed(true);
	group_directory_btn->connect("toggled", callable_mp(this, &FindInFilesPanel2::_toggle_grouping).bind(FindInFilesPanelTab::GroupingModeFlags::DIRECTORY));
	buttons_vbox->add_child(group_directory_btn);

	group_files_btn = memnew(Button);
	group_files_btn->set_tooltip_text(TTR("Group by files"));
	group_files_btn->set_flat(true);
	group_files_btn->set_toggle_mode(true);
	group_files_btn->set_pressed(true);
	group_files_btn->connect("toggled", callable_mp(this, &FindInFilesPanel2::_toggle_grouping).bind(FindInFilesPanelTab::GroupingModeFlags::FILE));
	buttons_vbox->add_child(group_files_btn);

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
	tabs->connect(SNAME("tab_button_pressed"), callable_mp(this, &FindInFilesPanel2::_on_tab_button_pressed));
	tabs->connect(SNAME("tab_changed"), callable_mp(this, &FindInFilesPanel2::_on_tab_changed));

	main_hbox->add_child(tabs);
}
