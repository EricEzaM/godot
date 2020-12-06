/*************************************************************************/
/*  editor_node.h                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#ifndef EDITOR_NODE_H
#define EDITOR_NODE_H

#include "editor/editor_data.h"
#include "editor/editor_export.h"
#include "editor/editor_folding.h"
#include "editor/editor_run.h"
#include "editor/inspector_dock.h"
#include "editor/property_editor.h"
#include "editor/scene_tree_dock.h"

typedef void (*EditorNodeInitCallback)();
typedef void (*EditorPluginInitializeCallback)();
typedef bool (*EditorBuildCallback)();

class AcceptDialog;
class AudioStreamPreviewGenerator;
class BackgroundProgress;
class CenterContainer;
class ConfirmationDialog;
class Control;
class DependencyEditor;
class DependencyErrorDialog;
class EditorAbout;
class EditorExport;
class EditorFeatureProfileManager;
class EditorFileServer;
class EditorInspector;
class EditorLayoutsDialog;
class EditorLog;
class EditorPlugin;
class EditorPluginList;
class EditorQuickOpen;
class EditorResourcePreview;
class EditorRunNative;
class EditorSettingsDialog;
class ExportTemplateManager;
class FileSystemDock;
class HSplitContainer;
class ImportDock;
class MenuButton;
class NodeDock;
class OrphanResourcesDialog;
class Panel;
class PanelContainer;
class PluginConfigDialog;
class ProgressDialog;
class ProjectExportDialog;
class ProjectSettingsEditor;
class ScriptCreateDialog;
class TabContainer;
class Tabs;
class TextureProgress;
class Button;
class VSplitContainer;
class Window;
class SubViewport;

class EditorNode : public Node {
	GDCLASS(EditorNode, Node);

public:
	enum DockSlot {
		DOCK_SLOT_LEFT_UL,
		DOCK_SLOT_LEFT_BL,
		DOCK_SLOT_LEFT_UR,
		DOCK_SLOT_LEFT_BR,
		DOCK_SLOT_RIGHT_UL,
		DOCK_SLOT_RIGHT_BL,
		DOCK_SLOT_RIGHT_UR,
		DOCK_SLOT_RIGHT_BR,
		DOCK_SLOT_MAX
	};

	struct ExecuteThreadArgs {
		String path;
		List<String> args;
		String output;
		Thread *execute_output_thread;
		Mutex execute_output_mutex;
		int exitcode;
		volatile bool done;
	};

private:
	enum {
		HISTORY_SIZE = 64
	};

	enum MenuOptions {
		FILE_NEW_SCENE,
		FILE_NEW_INHERITED_SCENE,
		FILE_OPEN_SCENE,
		FILE_SAVE_SCENE,
		FILE_SAVE_AS_SCENE,
		FILE_SAVE_ALL_SCENES,
		FILE_SAVE_BEFORE_RUN,
		FILE_SAVE_AND_RUN,
		FILE_SHOW_IN_FILESYSTEM,
		FILE_IMPORT_SUBSCENE,
		FILE_EXPORT_PROJECT,
		FILE_EXPORT_MESH_LIBRARY,
		FILE_INSTALL_ANDROID_SOURCE,
		FILE_EXPLORE_ANDROID_BUILD_TEMPLATES,
		FILE_EXPORT_TILESET,
		FILE_OPEN_RECENT,
		FILE_OPEN_OLD_SCENE,
		FILE_QUICK_OPEN,
		FILE_QUICK_OPEN_SCENE,
		FILE_QUICK_OPEN_SCRIPT,
		FILE_OPEN_PREV,
		FILE_CLOSE,
		FILE_CLOSE_OTHERS,
		FILE_CLOSE_RIGHT,
		FILE_CLOSE_ALL,
		FILE_CLOSE_ALL_AND_QUIT,
		FILE_CLOSE_ALL_AND_RUN_PROJECT_MANAGER,
		FILE_QUIT,
		FILE_EXTERNAL_OPEN_SCENE,
		EDIT_UNDO,
		EDIT_REDO,
		EDIT_RELOAD_SAVED_SCENE,
		TOOLS_ORPHAN_RESOURCES,
		TOOLS_CUSTOM,
		RESOURCE_SAVE,
		RESOURCE_SAVE_AS,
		RUN_PLAY,

		RUN_STOP,
		RUN_PLAY_SCENE,
		RUN_PLAY_CUSTOM_SCENE,
		RUN_SETTINGS,
		RUN_PROJECT_DATA_FOLDER,
		RUN_PROJECT_MANAGER,
		RUN_VCS_SETTINGS,
		RUN_VCS_SHUT_DOWN,
		SETTINGS_UPDATE_CONTINUOUSLY,
		SETTINGS_UPDATE_WHEN_CHANGED,
		SETTINGS_UPDATE_ALWAYS,
		SETTINGS_UPDATE_CHANGES,
		SETTINGS_UPDATE_SPINNER_HIDE,
		SETTINGS_PREFERENCES,
		SETTINGS_LAYOUT_SAVE,
		SETTINGS_LAYOUT_DELETE,
		SETTINGS_LAYOUT_DEFAULT,
		SETTINGS_EDITOR_DATA_FOLDER,
		SETTINGS_EDITOR_CONFIG_FOLDER,
		SETTINGS_MANAGE_EXPORT_TEMPLATES,
		SETTINGS_MANAGE_FEATURE_PROFILES,
		SETTINGS_PICK_MAIN_SCENE,
		SETTINGS_TOGGLE_CONSOLE,
		SETTINGS_TOGGLE_FULLSCREEN,
		SETTINGS_HELP,
		SCENE_TAB_CLOSE,

		EDITOR_SCREENSHOT,
		EDITOR_OPEN_SCREENSHOT,

		HELP_SEARCH,
		HELP_DOCS,
		HELP_QA,
		HELP_REPORT_A_BUG,
		HELP_SEND_DOCS_FEEDBACK,
		HELP_COMMUNITY,
		HELP_ABOUT,

		SET_VIDEO_DRIVER_SAVE_AND_RESTART,

		GLOBAL_NEW_WINDOW,
		GLOBAL_SCENE,

		IMPORT_PLUGIN_BASE = 100,

		TOOL_MENU_BASE = 1000
	};

	SubViewport *scene_root; //root of the scene being edited

	PanelContainer *scene_root_parent;
	Control *theme_base;
	Control *gui_base;
	VBoxContainer *main_vbox;
	OptionButton *video_driver;

	ConfirmationDialog *video_restart_dialog;

	int video_driver_current;
	String video_driver_request;
	void _video_driver_selected(int);
	void _update_video_driver_color();

	// Split containers

	HSplitContainer *left_l_hsplit;
	VSplitContainer *left_l_vsplit;
	HSplitContainer *left_r_hsplit;
	VSplitContainer *left_r_vsplit;
	HSplitContainer *main_hsplit;
	HSplitContainer *right_hsplit;
	VSplitContainer *right_l_vsplit;
	VSplitContainer *right_r_vsplit;

	VSplitContainer *center_split;

	// To access those easily by index
	Vector<VSplitContainer *> vsplits;
	Vector<HSplitContainer *> hsplits;

	// Main tabs

	Tabs *scene_tabs;
	PopupMenu *scene_tabs_context_menu;
	Panel *tab_preview_panel;
	TextureRect *tab_preview;
	int tab_closing;

	bool exiting;
	bool dimmed;

	int old_split_ofs;
	VSplitContainer *top_split;
	HBoxContainer *bottom_hb;
	Control *vp_base;

	HBoxContainer *menu_hb;
	Control *viewport;
	MenuButton *file_menu;
	MenuButton *project_menu;
	MenuButton *debug_menu;
	MenuButton *settings_menu;
	MenuButton *help_menu;
	PopupMenu *tool_menu;
	Button *export_button;
	Button *prev_scene;
	Button *play_button;
	Button *pause_button;
	Button *stop_button;
	Button *run_settings_button;
	Button *play_scene_button;
	Button *play_custom_scene_button;
	Button *search_button;
	TextureProgress *audio_vu;

	Timer *screenshot_timer;

	PluginConfigDialog *plugin_config_dialog;

	RichTextLabel *load_errors;
	AcceptDialog *load_error_dialog;

	RichTextLabel *execute_outputs;
	AcceptDialog *execute_output_dialog;

	Ref<Theme> theme;

	PopupMenu *recent_scenes;
	SceneTreeDock *scene_tree_dock;
	InspectorDock *inspector_dock;
	NodeDock *node_dock;
	ImportDock *import_dock;
	FileSystemDock *filesystem_dock;
	EditorRunNative *run_native;

	ConfirmationDialog *confirmation;
	ConfirmationDialog *save_confirmation;
	ConfirmationDialog *import_confirmation;
	ConfirmationDialog *pick_main_scene;
	AcceptDialog *accept;
	EditorAbout *about;
	AcceptDialog *warning;

	int overridden_default_layout;
	Ref<ConfigFile> default_layout;
	PopupMenu *editor_layouts;
	EditorLayoutsDialog *layout_dialog;

	ConfirmationDialog *custom_build_manage_templates;
	ConfirmationDialog *install_android_build_template;
	ConfirmationDialog *remove_android_build_template;

	EditorSettingsDialog *settings_config_dialog;
	ProjectSettingsEditor *project_settings_editor;
	PopupMenu *vcs_actions_menu;
	EditorFileDialog *file;
	ExportTemplateManager *export_template_manager;
	EditorFeatureProfileManager *feature_profile_manager;
	EditorFileDialog *file_templates;
	EditorFileDialog *file_export_lib;
	EditorFileDialog *file_script;
	CheckBox *file_export_lib_merge;
	String current_path;
	MenuButton *update_spinner;

	String defer_load_scene;
	Node *_last_instanced_scene;

	EditorLog *log;
	CenterContainer *tabs_center;
	EditorQuickOpen *quick_open;
	EditorQuickOpen *quick_run;

	HBoxContainer *main_editor_button_vb;
	Vector<Button *> main_editor_buttons;
	Vector<EditorPlugin *> editor_table; // shit name

	AudioStreamPreviewGenerator *preview_gen;
	ProgressDialog *progress_dialog;
	BackgroundProgress *progress_hb;

	DependencyErrorDialog *dependency_error;
	DependencyEditor *dependency_fixer;
	OrphanResourcesDialog *orphan_resources;
	ConfirmationDialog *open_imported;
	Button *new_inherited_button;
	String open_import_request;

	Vector<Control *> floating_docks;

	TabContainer *dock_slot[DOCK_SLOT_MAX];
	Rect2 dock_select_rect[DOCK_SLOT_MAX];
	int dock_select_rect_over;
	PopupPanel *dock_select_popup;
	Control *dock_select;
	Button *dock_float;
	Button *dock_tab_move_left;
	Button *dock_tab_move_right;
	int dock_popup_selected;
	Timer *dock_drag_timer;
	bool docks_visible;

	HBoxContainer *tabbar_container;
	Button *distraction_free;
	Button *scene_tab_add;

	bool scene_distraction;
	bool script_distraction;

	String _tmp_import_path;

	EditorExport *editor_export;

	Object *current;
	Ref<Resource> saving_resource;

	bool _playing_edited;
	String run_custom_filename;
	bool reference_resource_mem;
	bool save_external_resources_mem;
	uint64_t saved_version;
	uint64_t last_checked_version;
	bool unsaved_cache;
	String open_navigate;
	bool changing_scene;
	bool waiting_for_first_scan;

	bool waiting_for_sources_changed;

	uint32_t update_spinner_step_msec;
	uint64_t update_spinner_step_frame;
	int update_spinner_step;

	Vector<EditorPlugin *> editor_plugins;
	EditorPlugin *editor_plugin_screen;
	EditorPluginList *editor_plugins_over;
	EditorPluginList *editor_plugins_force_over;
	EditorPluginList *editor_plugins_force_input_forwarding;

	EditorHistory editor_history;
	EditorData editor_data;
	EditorRun editor_run;
	EditorSelection *editor_selection;
	ProjectExportDialog *project_export;
	EditorResourcePreview *resource_preview;
	EditorFolding editor_folding;

	struct BottomPanelItem {
		String name;
		Control *control;
		Button *button;
	};

	Vector<BottomPanelItem> bottom_panel_items;

	PanelContainer *bottom_panel;
	HBoxContainer *bottom_panel_hb;
	HBoxContainer *bottom_panel_hb_editors;
	VBoxContainer *bottom_panel_vb;
	Label *version_label;
	Button *bottom_panel_raise;

	void _bottom_panel_raise_toggled(bool);

	EditorInterface *editor_interface;

	void _bottom_panel_switch(bool p_enable, int p_idx);

	String external_file;
	List<String> previous_scenes;
	bool opening_prev;

	// FIXME
	// too much stuff... too generic name. needs refactor.
	void _dialog_action(String p_file);

	/**
	 * Edits the current object according to the "current" object in EditorHistory.
	*/
	void _edit_current();

	// Shows accept dialogs when save or load errors occur on a particular file.
	void _dialog_display_save_error(String p_file, Error p_error);
	void _dialog_display_load_error(String p_file, Error p_error);

	/// Cache of the currently selected option - typically used when dialogs are need to be used.
	int current_option;
	/**
	 * Activates a menu option without confirming it.
	 * @param p_option An option from MenuOptions.
	 */
	void _menu_option(int p_option);
	/**
	 * Activates the current menu option with confirming it.
	 * @param p_option An option from MenuOptions.
	 */
	void _menu_confirm_current();
	/**
	 * Activates a menu option.
	 * @param p_option An option from MenuOptions.
	 * @param p_confirmed True if the user has 'confirmed' the option, for example via a confirmation dialog. False otherwise.
	 */
	void _menu_option_confirm(int p_option, bool p_confirmed);

	/**
	 * Takes a screenshot of the editor.
	 * @param p_use_utc True if the the timestamp on the file should be in UTC and not the local timezone of the OS.
	 */
	void _screenshot(bool p_use_utc = false);

	/**
	 * Activate a tool menu option in Project -> Tools submenu.
	 * @param p_idx The index of the submenu item to activate.
	 */
	void _tool_menu_option(int p_idx);

	// FIXME: Check if below methods can be refactored out.

	/**
	 * Updates the file menu when it is opened.
	 */
	void _update_file_menu_opened();

	/**
	 * Updates the file menu when it is closed.
	 */
	void _update_file_menu_closed();

	// ?
	void _on_plugin_ready(Object *p_script, const String &p_activate_name);

	/**
	 * Callbacks for when EditorFileSystem notifies that the filesystem changed, resources were reimported or changed,
	 * or sources changed.
	 */

	void _filesystem_changed();
	void _resources_reimported(const Vector<String> &p_resources);
	void _resources_changed(const Vector<String> &p_resources);
	void _sources_changed(bool p_exist);

	/**
	 * Opens the next main window (i.e. 2D, 3D, Script, etc).
	 */
	void _editor_select_next();

	/**
	 * Opens the previous main window (i.e. 2D, 3D, Script, etc).
	 */
	void _editor_select_prev();

	/**
	 * Displays the selected editor.
	 * @param p_which A value from the EditorTable enum, corresponding to the editor to be displayed.
	 */
	void _editor_select(int p_which);

	/**
	 * Saves the editor plugin states for the chosen scene.
	 * @param p_file The filename of the scene.
	 * @param p_idx The index of the scene tab which corresponds to the scene. -1 is the currently edited scene.
	 */
	void _save_editor_state_for_scene(const String &p_file, int p_idx = -1);

	/**
	 * Applies the editor plugin states for the chosen scene.
	 * @param p_file The filename of the scene.
	 */
	void _apply_editor_state_for_scene(const String &p_file);

	/**
	 * Updates the title of the Window to the application (project) name and the name of the currently edited scene.
	 */
	void _update_title();

	/**
	 * Rebuilds the scene tabs.
	 */
	void _update_scene_tabs();

	/**
	 * Handled selecting version control plugin option.
	 * @param p_idx 
	*/
	// FIXME: This should NOT be here!
	void _version_control_menu_option(int p_idx);

	// ?
	int _save_external_resources();

	// Move this to a utility class - its functionality is duplicated in SceneTreeEditor.
	bool _validate_scene_recursive(const String &p_filename, Node *p_node);

	// FIXME fix this this saving shiz! Confusing af!!
	int _next_unsaved_scene(bool p_valid_filename, int p_start = 0);

	// does too much stuff... needs refactor.
	void _discard_changes(const String &p_str = String());

	/**
	 * Inherits the selected scene and opens the new inherited scene.
	 * @param p_file The filepath of the scene to be inherited.
	*/
	void _inherit_request(String p_file);

	/**
	 * Instances the selected scenes in the currently edited scene, on the currently selected node.
	 * @param p_files 
	*/
	void _instance_request(const Vector<String> &p_files);

	/**
	 * Deal with the "top" (shit name) editor plugins. Like the ones that let you click to add/remove/edit points on polygons and stuff in the 2d view.
	*/

	void _display_top_editors(bool p_display);
	void _set_top_editors(Vector<EditorPlugin *> p_editor_plugins_over);
	void _set_editing_top_editors(Object *p_current_object);

	/**
	 * Opens the selected files from the EditorQuickOpen after it is confirmed. (i.e. this is a callback)
	*/
	void _quick_opened();

	/**
	 * Runs the selected scene from the EditorQuickOpen after it is confirmed. (i.e. this is a callback)
	*/
	void _quick_run();

	/**
	 * Runs the scene.
	 * @param p_current Whether to run the currently open scene.
	 * @param p_custom The filepath of the scene to run, if the current is not run.
	*/
	void _run(bool p_current = false, const String &p_custom = "");

	// ?
	// Not quite sure, seems like this runs in a native format, like debugging HTML, Android games, etc?
	void _run_native(const Ref<EditorExportPreset> &p_preset);

	/**
	 * Adds the provided scene path to the recent scenes in the project metadata file.
	 * @param p_scene Scene path.
	*/
	void _add_to_recent_scenes(const String &p_scene);

	/**
	 * Updates the "recent scenes" popup menu with the scenes from the project metadata file.
	*/
	void _update_recent_scenes();

	/**
	 * Opens a scene from the recent scenes file list.
	 * @param p_idx The index of the popup menu item which represents the scene to open.
	*/
	void _open_recent_scene(int p_idx);

	/**
	 * Selects the scene from the global menu callback.
	 * ONLY ON MAC OS. CALLBACK ONLY.
	 * @param p_tag 
	*/
	void _global_menu_scene(const Variant &p_tag);

	/**
	 * Opens a new project manager from the global menu.
	 * ONLY ON MAC OS. CALLBACK ONLY.
	*/
	void _global_menu_new_window(const Variant &);

	/**
	 * Handles when files are dropped onto the editor from the filesystem. Copies the files into the
	 * project directory at the currently selected FileSystemDock directory.
	 * @param p_files List of files to copy.
	*/
	void _dropped_files(const Vector<String> &p_files);

	/**
	 * Recursively adds all files to the selected path. Recursive so that files in sub-directories are correctly copied.
	 * This is ONLY USED in _dropped_files.
	 * @param p_files List of files to copy.
	 * @param to_path Target path.
	*/
	void _add_dropped_files_recursive(const Vector<String> &p_files, String to_path);

	// random ass property, clean it up, move it somewhere better!!!
	String _recent_scene;

	/**
	 * Saves docks and quits the editor.
	*/
	void _exit_editor();

	/**
	 * Standard unhandled input callback.
	*/
	// FIXME GROUP THESE TOGETHER
	void _unhandled_input(const Ref<InputEvent> &p_event);

	/**
	 * Displays an error dialog after resource load error. Used as a callback only for ResourceLoader::set_error_notify_func
	*/
	static void _load_error_notify(void *p_ud, const String &p_text);

	/**
	 * File Dialogs for the editor.
	*/
	Set<FileDialog *> file_dialogs;
	Set<EditorFileDialog *> editor_file_dialogs;

	/**
	 * Cache of the icons for the editor theme so that icons do not need to be retreieved manually every time.
	*/
	Map<String, Ref<Texture2D>> icon_type_cache;
	void _build_icon_type_cache();

	bool _initializing_addons;
	Map<String, EditorPlugin *> plugin_addons;

	/**
	 * Methods for registration and handling of FileDialogs.
	 * These are called on filedialog Construction & Destruction, and
	 * just add the dialogs to a list internally so things can be changed on them in bulk.
	*/
	static Ref<Texture2D> _file_dialog_get_icon(const String &p_path);
	static void _file_dialog_register(FileDialog *p_dialog);
	static void _file_dialog_unregister(FileDialog *p_dialog);
	static void _editor_file_dialog_register(EditorFileDialog *p_dialog);
	static void _editor_file_dialog_unregister(EditorFileDialog *p_dialog);

	/**
	 * Removes the scene from the scene tabs.
	 * @param p_change_tab Not sure
	*/
	void _remove_edited_scene(bool p_change_tab = true);
	void _remove_scene(int index, bool p_change_tab = true);

	/**
	 * Save the resources on all nodes in the provided scene.
	 * @param scene The root node of the scene to save the resources for.
	 * @param processed A map of the resources and whether or not they were saved.
	 * @param flags Flags passed to ResourceSaver::save.
	*/
	void _save_edited_subresources(Node *scene, Map<RES, bool> &processed, int32_t flags);

	/**
	 * Saves the resouces on the provided object.
	 * @param obj The object to save resources for.
	 * @param processed A map of the resources and whether or not they were saved.
	 * @param flags Flags passed to ResourceSaver::save.
	 * @return True if any built-in resources were saved. False if resource was a file or was unchanged.
	*/
	bool _find_and_save_edited_subresources(Object *obj, Map<RES, bool> &processed, int32_t flags);

	/**
	 * Saves the resource and all its subresources.
	 * @param p_res The resource to be saved.
	 * @param processed A map of the resources and whether or not they were saved.
	 * @param flags Flags passed to ResourceSaver::save.
	 * @return True if any built-in resources were saved. False if resource was a file or was unchanged.
	*/
	bool _find_and_save_resource(RES p_res, Map<RES, bool> &processed, int32_t flags);

	// Called after filesystem change was detected. Not quite sure what this actually does, and why the logic inside this method is they way it is
	void _mark_unsaved_scenes();

	/**
	 * Gets the counts of 2D and 3D nodes in a node tree.
	 * @param p_node The node at the top of the not tree to count.
	 * @param count_2d [out] Number of 2D nodes.
	 * @param count_3d [out] Number of 3D nodes.
	*/
	void _find_node_types(Node *p_node, int &count_2d, int &count_3d);

	/**
	 * Saves the scene at tab idx to the filepath provided.
	 * @param p_file The filepath to save the scene to.
	 * @param p_idx The index of the tab with the scene to be saved. -1 = currently edited scene. NOTE: Preview can only be made with p_idx = -1.
	*/
	void _save_scene_with_preview(String p_file, int p_idx = -1);

	/**
	 * Saves the scene.
	 * @param p_file The file path to save the scene to.
	 * @param idx The index of the scene tab which has the scene to be saved. -1 = currently open tab.
	*/
	void _save_scene(String p_file, int idx = -1);

	/**
	 * Loops through edited scenes and saves them.
	*/
	void _save_all_scenes();

	/**
	 * Depencency errors from ResourceLoader
	*/
	Map<String, Set<String>> dependency_errors;
	static void _dependency_error_report(void *ud, const String &p_path, const String &p_dep, const String &p_type) {
		EditorNode *en = (EditorNode *)ud;
		if (!en->dependency_errors.has(p_path)) {
			en->dependency_errors[p_path] = Set<String>();
		}
		en->dependency_errors[p_path].insert(p_dep + "::" + p_type);
	}

	/**
	 * To do with export preset called from main.cpp
	*/
	bool cmdline_export_mode;
	struct ExportDefer {
		String preset;
		String path;
		bool debug;
		bool pack_only;
	} export_defer;

	static EditorNode *singleton;

	static Vector<EditorNodeInitCallback> _init_callbacks;

	/**
	 * Finds if the scene at p_path exists as a child of p_node
	 * @param p_node The node to check if the scene exists in.
	 * @param p_path The path of the Scene.
	 * @return True if the scene at p_path exists in the hierarchy of p_node.
	*/
	bool _find_scene_in_use(Node *p_node, const String &p_path) const;

	/**
	 * Toggles visibility of the dock containers based on whether or not they have any items.
	*/
	void _update_dock_containers();

	/// DOCK STUFF

	/**
	 * CALLBACK ONLY.
	 * Handle input from the dock selection popup.
	 * @param p_input The input event.
	*/
	void _dock_select_input(const Ref<InputEvent> &p_input);
	void _dock_move_left();
	void _dock_move_right();
	void _dock_select_draw();
	void _dock_pre_popup(int p_which);
	void _dock_split_dragged(int ofs);
	void _dock_popup_exit();
	void _dock_floating_close_request(Control *p_control);
	void _dock_make_float();

	void _save_docks();
	void _load_docks();
	void _save_docks_to_config(Ref<ConfigFile> p_layout, const String &p_section);
	void _load_docks_from_config(Ref<ConfigFile> p_layout, const String &p_section);
	void _update_dock_slots_visibility();
	void _dock_tab_changed(int p_tab);

	/// SCENE TAB STUFF

	void _scene_tab_changed(int p_tab);
	void _scene_tab_closed(int p_tab, int option = SCENE_TAB_CLOSE);
	void _scene_tab_hover(int p_tab);
	void _scene_tab_exit();
	void _scene_tab_input(const Ref<InputEvent> &p_input);
	void _reposition_active_tab(int idx_to);
	void _scene_tab_script_edited(int p_tab);

	/**
	 * Callback after the resource preview has loaded when hovering the Tab.
	*/
	void _thumbnail_done(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview, const Variant &p_udata);

	/**
	 * Gets the state of the editor.
	 * @return Dictionary with:
	 * * "main_tab" Index of current editor selected.
	 * * "scene_tree_offset" Scroll amount of the scene tree dock.
	 * * "property_edit_offset" Scroll amount of the inspector.
	 * * "saved_version" Version of the current scene. (??? Not certain of this one)
	 * * "node_filter" Filter text in the scene tree dock.
	*/
	Dictionary _get_main_scene_state();
	void _set_main_scene_state(Dictionary p_state, Node *p_for_scene);

	/**
	 * Gets the index of the current main editor.
	 * @return The index of the item in editor_table which is currently the main editor.
	*/
	int _get_current_main_editor();

	bool restoring_scenes;

	/**
	 * Saves the list of open scenes to the project settings directory editor_config.cfg file.
	 * @param p_layout The config file to save the data to.
	 * @param p_section The name of the section to save the list to.
	*/
	void _save_open_scenes_to_config(Ref<ConfigFile> p_layout, const String &p_section);

	/**
	 * Get the scenes from the passed config file and load them.
	 * @param p_layout The config file to read the data from.
	 * @param p_section The section which contains the open_scenes list.
	*/
	void _load_open_scenes_from_config(Ref<ConfigFile> p_layout, const String &p_section);

	/**
	 * Updates the layouts menu, adding new items for each layout saved in the editor settings.
	*/
	void _update_layouts_menu();
	/**
	 * Handles a layout option being selected.
	 * @param p_id The id of the item selected.
	*/
	void _layout_menu_option(int p_id);

	/**
	 * Updates the project settings with the list of enabled addons.
	*/
	void _update_addon_config();

	/**
	 * CALLBACK For when FileAccess is unable to close a file.
	*/
	static void _file_access_close_error_notify(const String &p_str);

	/**
	 * Toggles the distraction free mode.
	*/
	void _toggle_distraction_free_mode();

	// FIXME This should be moved somewhere better
	enum {
		MAX_INIT_CALLBACKS = 128,
		MAX_BUILD_CALLBACKS = 128
	};

	/**
	 * Callback for after trying to edit an imported scene, and the user selects to inherit the scene.
	 * @param p_action UNUSED
	*/
	void _inherit_imported(const String &p_action);

	/**
	 * Callback for after trying to edit an imported scene, and the user selects to open the scene anyway.
	*/
	void _open_imported();

	static int plugin_init_callback_count;
	static EditorPluginInitializeCallback plugin_init_callbacks[MAX_INIT_CALLBACKS];
	void _save_default_environment();

	static int build_callback_count;
	static EditorBuildCallback build_callbacks[MAX_BUILD_CALLBACKS];

	/**
	 * Updates the spinner behaviour and visibility based on the editor settings, and sets the OS low processer mode if required.
	*/
	void _update_update_spinner();

	Vector<Ref<EditorResourceConversionPlugin>> resource_conversion_plugins;

	PrintHandlerList print_handler;
	static void _print_handler(void *p_this, const String &p_string, bool p_error);

	/**
	 * Callbacks for after a resource is saved or loaded from ResourceSaver or ResourceLoader
	*/

	static void _resource_saved(RES p_resource, const String &p_path);
	static void _resource_loaded(RES p_resource, const String &p_path);

	/**
	 * Callback for after the feature profile is changed.
	*/
	void _feature_profile_changed();

	/**
	 * Checks if class or class editor is disabled by feature profile.
	 * @param p_class Class name.
	 * @return True if class is disabled. 
	*/
	bool _is_class_editor_disabled_by_feature_profile(const StringName &p_class);

	/**
	 * Loads the icon at the provided path and resizes it to be the correct size for an editor class icon.
	 * @param p_path Path to the image to be used for the icon.
	 * @return Reference to the ImageTexture of the resize icon.
	*/
	Ref<ImageTexture> _load_custom_class_icon(const String &p_path) const;

protected:
	void _notification(int p_what);

	static void _bind_methods();

protected:
	friend class FileSystemDock;

	/**
	 * Gets the current selected scene tab.
	 * @return The tab index.
	*/
	int get_current_tab();

	/**
	 * Sets the current selected scene tab.
	 * @param p_tab The tab index to select.
	*/
	void set_current_tab(int p_tab);

public:
	// ?
	bool call_build();

	/**
	 * Adds a callback method which will be called after all editor plugins have been constructed.
	 * @param p_callback The method which matches the EditorPluinInitializeCallback signature.
	*/
	static void add_plugin_init_callback(EditorPluginInitializeCallback p_callback);

	/**
	 * Enum containing the valid "main screen" options.
	*/
	// FIXME bad name, bad location.
	enum EditorTable {
		EDITOR_2D = 0,
		EDITOR_3D,
		EDITOR_SCRIPT,
		EDITOR_ASSETLIB
	};

	/**
	 * Displays the selected editor.
	 * @param p_which A value from the EditorTable enum, corresponding to the editor to be displayed.
	 */
	void set_visible_editor(EditorTable p_table) { _editor_select(p_table); }

	/**
	 * Gets the EditorNode singleton.
	*/
	static EditorNode *get_singleton() { return singleton; }

	/**
	 * Gets the currently displayed editor plugin.
	*/
	EditorPlugin *get_editor_plugin_screen() { return editor_plugin_screen; }

	/**
	 * Gets the list of "top" editor plugins which are displayed in addition to the main editor plugin.
	 * @return EditorPluginList instance.
	*/
	EditorPluginList *get_editor_plugins_over() { return editor_plugins_over; }

	/**
	 * Gets the list of plugins which should always draw over the viewport, even if it does not handle the selected object type.
	 * @return EditorPluginList.
	*/
	EditorPluginList *get_editor_plugins_force_over() { return editor_plugins_force_over; }

	/**
	 * Gets the list of plugins which should always recieve input forwarding, even if it does not handle the selected object type.
	 * @return 
	*/
	EditorPluginList *get_editor_plugins_force_input_forwarding() { return editor_plugins_force_input_forwarding; }

	/**
	 * Gets the inspector.
	 * @return EditorInspector instance.
	*/
	EditorInspector *get_inspector() { return inspector_dock->get_inspector(); }

	/**
	 * Gets the addon area for the Inspector dock.
	 * @return Container instance.
	*/
	Container *get_inspector_dock_addon_area() { return inspector_dock->get_addon_area(); }

	/**
	 * Gets the scene tree dock.
	 * @return Instance of the create script dialog.
	*/
	ScriptCreateDialog *get_script_create_dialog() { return scene_tree_dock->get_script_create_dialog(); }

	/**
	 * Gets the project settings editor.
	 * @return Instance of the project settings editor.
	*/
	ProjectSettingsEditor *get_project_settings_editor() { return project_settings_editor; }

	/**
	 * Adds an editor plugin.
	 * @param p_editor Instance of the EditorPlugin to add.
	 * @param p_config_changed If true, will enable the plugin (calling enable_plugin).
	*/
	static void add_editor_plugin(EditorPlugin *p_editor, bool p_config_changed = false);

	/**
	 * Removes an editor plugin.
	 * @param p_editor Instance of the EditorPlugin to remove.
	 * @param p_config_changed If true, will disable the plugin (calling disable_plugin).
	*/
	static void remove_editor_plugin(EditorPlugin *p_editor, bool p_config_changed = false);

	/**
	 * Distinguishes filenames which are the same, but located in different folders.
	 * 
	 * @param p_full_paths A vector of the full paths of the files.
	 * @param[in, out] r_filenames A vector of only the filenames of the files.
	 * This is re-written where necessary and ambiguous filenames are replaced to remove ambiguity.
	 * E.g. if multiple filenames called `script.gd`, they will be changed to be `<folder name(s)>/script.gd`
	 */
	static void disambiguate_filenames(const Vector<String> p_full_paths, Vector<String> &r_filenames);

	/**
	 * Opens the dialog to select the base scene for creating an inherited scene.
	*/
	void new_inherited_scene() { _menu_option_confirm(FILE_NEW_INHERITED_SCENE, false); }

	// Shows/hides dock panels.
	void set_docks_visible(bool p_show);
	bool get_docks_visible() const; // UNUSED

	// Enables/disables distraction free mode (which really just hides or shows the docks)
	void set_distraction_free_mode(bool p_enter);
	bool is_distraction_free_mode_enabled() const;

	// Adds/removes a control to a specific dock.
	void add_control_to_dock(DockSlot p_slot, Control *p_control);
	void remove_control_from_dock(Control *p_control);

	// Enables/Disables plugins from scripts (addons), if it is an EditorPlugin.

	/**
	 * Enables or disables script EditorPlugins from the addons folder, if the inherit from EditorPlugin.
	 * @param p_addon The name of the addon.
	 * @param p_enabled Whether it should be enabled.
	 * @param p_config_changed Whether to immediately enable or disable the plugin.
	*/
	void set_addon_plugin_enabled(const String &p_addon, bool p_enabled, bool p_config_changed = false);
	bool is_addon_plugin_enabled(const String &p_addon) const;

	/**
	 * Propagates the call to edit the selected node to other controls, such as the Inspector, SceneTreeDock, etc.
	 * @param p_node The node instance to edit.
	*/
	void edit_node(Node *p_node);

	/**
	 * Edits the selected resource.
	 * @param p_resource A reference to the resource to edit.
	*/
	void edit_resource(const Ref<Resource> &p_resource) { inspector_dock->edit_resource(p_resource); };

	/**
	 * Opens a resource from disk, of the selected type.
	 * @param p_type Type of resource to open.
	*/
	void open_resource(const String &p_type) { inspector_dock->open_resource(p_type); };

	// Saves resources
	void save_resource_in_path(const Ref<Resource> &p_resource, const String &p_path);
	void save_resource(const Ref<Resource> &p_resource);
	void save_resource_as(const Ref<Resource> &p_resource, const String &p_at_path = String());

	/**
	 * Shortcut to the FILE_IMPORT_SUBSCENE option.
	*/
	void merge_from_scene() { _menu_option_confirm(FILE_IMPORT_SUBSCENE, false); }

	static bool has_unsaved_changes() { return singleton->unsaved_cache; }

	/**
	 * Gets the HBoxContainer containing the containers for the top menu buttons, main screen selection buttons, and the play/stop buttons.
	 * @return HBoxContainer.
	*/
	HBoxContainer *get_menu_hb() { return singleton->menu_hb; }

	void push_item(Object *p_object, const String &p_property = "", bool p_inspector_only = false);

	/**
	 * Enables sub-editor plugins which are allows to edit the provided object.
	 * @param p_object Object to edit.
	*/
	// FIXME I'm convinced these can be refactored. They are not used in very many places AT ALL. In fact, edit_item is only used
	// directly with nullptr, which just hides the top editors.
	void edit_item(Object *p_object);
	void edit_item_resource(RES p_resource);

	// Does object have editors which are enabled when the object is edited.
	bool item_has_editor(Object *p_object);

	// Hides the "top" (i.e. sub) editors.
	// FIXME Shit name, just like the property. Needs to be changed
	void hide_top_editors();

	// Dubious usefulness...
	void select_editor_by_name(const String &p_name);

	/**
	 * Loads the scene at the requested path. Will remove the scene from "Previous Scenes" if it appears in that list.
	 * @param p_path Path of scene to open.
	*/
	void open_request(const String &p_path);

	// Essentially "is changing scene tabs" from what I can tell.
	bool is_changing_scene() const;

	// Gets the EditorLog instance.
	static EditorLog *get_log() { return singleton->log; }

	// FIXME Hmm a variable called "viewport" which is actually a HBoxContainer and stored as a control, and NOT a Viewport type. This won't cause confusion...
	// Shit name, fix it.
	// Also, it is used in EditorExport when it should probabaly use get_gui_base() instead.
	Control *get_viewport();

	// Sets the scene to be edited
	void set_edited_scene(Node *p_scene);

	// Gets the currently edited scene
	Node *get_edited_scene() { return editor_data.get_edited_scene_root(); }

	// Gets the subviewport which acts as the scene root for editing scenes.
	SubViewport *get_scene_root() { return scene_root; } //root of the scene being edited

	// Opens the dependency editor dialog for the object at the provided file path.
	void fix_dependencies(const String &p_for_file);

	/**
	 * Creates a new scene tab
	 * @return The index of the tab with the new scene
	*/
	int new_scene();

	/**
	 * Loads a scene for editing.
	 * @param p_scene The path of the scene to load.
	 * @param p_ignore_broken_deps True if scene should be loaded despite missing/broken dependencies.
	 * @param p_set_inherited Only used when creating inherited scenes. If true, will make a new scene inherited to the scene at the path.
	 * @param p_clear_errors True if load errors should be cleared before starting to load the scene.
	 * @param p_force_open_imported True if scenes which were imported (not native Godot scenes?) should be opened anyway.
	 * @return Error status.
	*/
	Error load_scene(const String &p_scene, bool p_ignore_broken_deps = false, bool p_set_inherited = false, bool p_clear_errors = true, bool p_force_open_imported = false);

	/**
	 * Loads a resource for editing.
	 * @param p_resource The path of the resource to load.
	 * @param p_ignore_broken_deps True if broken dependencies are to be ignored when importing.
	 * @return Error status.
	*/
	Error load_resource(const String &p_resource, bool p_ignore_broken_deps = false);

	/**
	 * Whether the scene is open or not.
	 * @param p_path Path to scene.
	 * @return True if scene is open in the editor.
	*/
	bool is_scene_open(const String &p_path);

	/**
	 * Sets the version of the currently edited scene.
	 * @param p_version Version to set it to.
	*/
	void set_current_version(uint64_t p_version);

	/**
	 * Sets the scene to edit based on the provided tab index
	 * @param p_idx Tab index of scene to make the currently edited scene.
	*/
	void set_current_scene(int p_idx);

	/**
	 * Gets the EditorData.
	 * @return EditorData.
	*/
	static EditorData &get_editor_data() { return singleton->editor_data; }

	/**
	 * Gets the EditorFolding
	 * @return EditorFolding
	*/
	static EditorFolding &get_editor_folding() { return singleton->editor_folding; }

	/**
	 * Gets the EditorHistory
	 * @return EditorHistory
	*/
	EditorHistory *get_editor_history() { return &editor_history; }

	/**
	 * Requests the SceneTreeDock to instance the scenes at the paths in the provided vector.
	 * @param p_files Vector of paths to scenes to instance.
	*/
	void request_instance_scenes(const Vector<String> &p_files);

	// Get various Docks (which are always part of the UI)

	FileSystemDock *get_filesystem_dock();
	ImportDock *get_import_dock();
	SceneTreeDock *get_scene_tree_dock();
	InspectorDock *get_inspector_dock();

	/**
	 * Gets the UndoRedo from the EditorData
	 * @return UndoRedo
	*/
	static UndoRedo *get_undo_redo() { return &singleton->editor_data.get_undo_redo(); }

	/**
	 * Gets EditorSelection
	 * @return EditorSelection
	*/
	EditorSelection *get_editor_selection() { return editor_selection; }

	/**
	 * Call after all debug sessions stopped
	*/
	// FIXME Shit name, it is NOT a callback. Used in EditorDebuggerNode, but there a bunch of other stuff is also done on EditorNode. It should be merged into one method.
	void notify_all_debug_sessions_exited();

	/**
	 * Checks if the editor has the process with the provided id as a 'child' (i.e. the editor started the process).
	 * @param p_pid The process ID to check
	 * @return True if the process ID is a child of the editor.
	*/
	bool has_child_process(OS::ProcessID p_pid) const { return editor_run.has_child_process(p_pid); }

	/**
	 * Stops the process with the provided id.
	 * @param p_pid Process ID of the process to stop.
	*/
	void stop_child_process(OS::ProcessID p_pid);

	// Get the editor theme
	Ref<Theme> get_editor_theme() const { return theme; }

	// ??
	Ref<Script> get_object_custom_type_base(const Object *p_object) const;
	//  ??
	StringName get_object_custom_type_name(const Object *p_object) const;

	// Gets icon for object
	Ref<Texture2D> get_object_icon(const Object *p_object, const String &p_fallback = "Object") const;
	// Gets icon for class
	Ref<Texture2D> get_class_icon(const String &p_class, const String &p_fallback = "Object") const;

	/**
	 * Shows an accept dialog.
	 * @param p_text The label text to display in the dialog.
	 * @param p_title The text to be shown on the "Ok" button
	*/
	void show_accept(const String &p_text, const String &p_title);

	/**
	 * Shows a warning dialog if the dialog is in the tree, otherwise prints a warning to the log.
	 * @param p_text The label text to display in the dialog.
	 * @param p_title The title of the window.
	*/
	void show_warning(const String &p_text, const String &p_title = TTR("Warning!"));

	/**
	 * Copies a the warning text from the warning dialog to 
	 * @param p_str UNUSED
	*/
	// FIXME should not be public
	// FIXME param is unused
	void _copy_warning(const String &p_str);

	// To do with exporting... not sure exactly
	Error export_preset(const String &p_preset, const String &p_path, bool p_debug, bool p_pack_only);

	// Registering types
	static void register_editor_types();
	static void unregister_editor_types();

	// Seems to be only used to get theme stuff... maybe this can be made better.
	Control *get_gui_base() { return gui_base; }
	// Why use one of these over the other? Seems weird
	Control *get_theme_base() { return gui_base->get_parent_control(); }

	// Generic method for adding an error... could probably be better.
	static void add_io_error(const String &p_error);

	// Editor Progress window stuff

	static void progress_add_task(const String &p_task, const String &p_label, int p_steps, bool p_can_cancel = false);
	static bool progress_task_step(const String &p_task, const String &p_state, int p_step = -1, bool p_force_refresh = true);
	static void progress_end_task(const String &p_task);

	static void progress_add_task_bg(const String &p_task, const String &p_label, int p_steps);
	static void progress_task_step_bg(const String &p_task, int p_step = -1);
	static void progress_end_task_bg(const String &p_task);

	/**
	 * @brief Saves currently edited scene to the filepath specified.
	 * @param p_file Path to save the scene to.
	 * @param p_with_preview Whether to save with preview or not.
	*/
	void save_scene_to_path(String p_file, bool p_with_preview = true) {
		if (p_with_preview) {
			_save_scene_with_preview(p_file);
		} else {
			_save_scene(p_file);
		}
	}

	/**
	 * Determines if a particular scene is used in the currenly open scene
	 * @param p_path Path of the scene to check if it is used.
	 * @return True if scene at p_path is used in the currently edited scene.
	*/
	bool is_scene_in_use(const String &p_path); // UNUSED

	/**
	 * Saves the dock loayout after a brief delay.
	*/
	void save_layout();

	// Opens export template manager.
	// FIXME this could be removed by altering some logic in the ExportTemplateManager class I think.
	void open_export_template_manager();

	/**
	 * @brief Reload scene and all dependencies at specified path.
	 * @param p_path Path of scene to reload.
	*/
	void reload_scene(const String &p_path);

	// Is exiting...
	bool is_exiting() const { return exiting; }

	// Used in EditorDebugger node.
	Button *get_pause_button() { return pause_button; }

	/**
	 * Adds a control to the bottom panel (with output, debug, etc)
	 * @param p_text Text to display on button.
	 * @param p_item Control item to add.
	 * @return The button which will be used to show/hide the control.
	*/
	// FIXME make bottom panel it's own class...? Maybe other panels too?
	Button *add_bottom_panel_item(String p_text, Control *p_item);
	bool are_bottom_panels_hidden() const; // UNUSED
	void make_bottom_panel_item_visible(Control *p_item);
	void raise_bottom_panel_item(Control *p_item);
	void hide_bottom_panel();
	void remove_bottom_panel_item(Control *p_item);

	/**
	 * Gets the drag data for dragging a resource from a particular control.
	 * @param p_res The resource being dragged
	 * @param p_from Control where the drag initiated
	 * @return Dictionary with drag data.
	*/
	Variant drag_resource(const Ref<Resource> &p_res, Control *p_from);

	/**
	 * Gets the drag data for dragging folders and/or files.
	 * @param p_paths List of paths being dragged.
	 * @param p_from Control where the drag originated.
	 * @return Dictionary with drag data.
	*/
	Variant drag_files_and_dirs(const Vector<String> &p_paths, Control *p_from);

	// Tool menu stuff
	// FIXME Use callables?
	void add_tool_menu_item(const String &p_name, Object *p_handler, const String &p_callback, const Variant &p_ud = Variant());
	void add_tool_submenu_item(const String &p_name, PopupMenu *p_submenu);
	void remove_tool_menu_item(const String &p_name);

	/**
	 * Stops running and then saves all edited scenes.
	*/
	void save_all_scenes();

	/**
	 * Loops through a list of scene filenames (not paths?) and if they are currently being edited, saves them.
	 * @param p_scene_filenames List of filenames to save.
	*/
	// FIXME confirm this is filenames, not paths
	void save_scene_list(Vector<String> p_scene_filenames);

	/**
	 * Restarts the editor, saving the currently edited scene so it is reopened afterwards.
	*/
	void restart_editor();

	/**
	 * Dims the editor UI
	 * @param p_dimming If true, enables dimming of the editor.
	 * @param p_force_dim If true, forces the dimming of the editor even if the editor setting for dimming is disabled.
	*/
	// FIXME: rename p_dimming as it is confusing.
	void dim_editor(bool p_dimming, bool p_force_dim = false);
	bool is_editor_dimmed() const; // UNUSED

	// FIXME: remove the private method and just make it public...
	void edit_current() { _edit_current(); };

	// Updates keying (to do with the AnimationPlayerEditor and how you can create keys from the inspector dock.
	// This is only used to essentially alow AnimationPlayerEditor to communicate with the inspector dock.
	void update_keying() const { inspector_dock->update_keying(); };

	/**
	 * Determines if the editor has scened to load upon loading...?
	 * @return True if editor will open scenes after loading...?
	*/
	// FIXME Wtf does this do? Only called in main.cpp
	bool has_scenes_in_session();

	// Not sure... only used in Export.cpp
	int execute_and_show_output(const String &p_title, const String &p_path, const List<String> &p_arguments, bool p_close_on_ok = true, bool p_close_on_errors = false);

	EditorNode();
	~EditorNode();

	// Resource Conversion Plugin stuff
	// Add and remove methods are currently UNUSED
	void add_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin> &p_plugin);
	void remove_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin> &p_plugin);
	// This IS USED as Conversion Plugins are added to the list manually in the constructor.
	Vector<Ref<EditorResourceConversionPlugin>> find_resource_conversion_plugin(const Ref<Resource> &p_for_resource);

	// Adding callbacks.
	static void add_init_callback(EditorNodeInitCallback p_callback) { _init_callbacks.push_back(p_callback); }
	static void add_build_callback(EditorBuildCallback p_callback); // NOTE: Not currently used anywhere

	/**
	 * Ensures that a main scene is selected
	 * @param p_from_native True if the call is made from a native run.
	 * @return True if a main scene is set, false otherwise.
	*/
	bool ensure_main_scene(bool p_from_native);

	/**
	 * Plays the Main Scene
	*/
	void run_play();

	/**
	 * Plays the currently edited scene.
	*/
	void run_play_current();

	/**
	 * Plays the scene at the provided path
	 * @param p_custom Path of the scene to play
	*/
	void run_play_custom(const String &p_custom);

	/**
	 * Stops running the playing scene.
	*/
	void run_stop();

	/**
	 * Is the editor currently running a scene.
	 * @return True if running or paused.
	*/
	bool is_run_playing() const;

	/**
	 * Gets the Filename or filepath (???) of the running scene.
	 * @return String with the path/name of playing scene.
	*/
	String get_run_playing_scene() const;
};

struct EditorProgress {
	String task;
	bool step(const String &p_state, int p_step = -1, bool p_force_refresh = true) { return EditorNode::progress_task_step(task, p_state, p_step, p_force_refresh); }
	EditorProgress(const String &p_task, const String &p_label, int p_amount, bool p_can_cancel = false) {
		EditorNode::progress_add_task(p_task, p_label, p_amount, p_can_cancel);
		task = p_task;
	}
	~EditorProgress() { EditorNode::progress_end_task(task); }
};

class EditorPluginList : public Object {
private:
	Vector<EditorPlugin *> plugins_list;

public:
	void set_plugins_list(Vector<EditorPlugin *> p_plugins_list) {
		plugins_list = p_plugins_list;
	}

	Vector<EditorPlugin *> &get_plugins_list() {
		return plugins_list;
	}

	void make_visible(bool p_visible);
	void edit(Object *p_object);
	bool forward_gui_input(const Ref<InputEvent> &p_event);
	void forward_canvas_draw_over_viewport(Control *p_overlay);
	void forward_canvas_force_draw_over_viewport(Control *p_overlay);
	bool forward_spatial_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event, bool serve_when_force_input_enabled);
	void forward_spatial_draw_over_viewport(Control *p_overlay);
	void forward_spatial_force_draw_over_viewport(Control *p_overlay);
	void add_plugin(EditorPlugin *p_plugin);
	void remove_plugin(EditorPlugin *p_plugin);
	void clear();
	bool empty();

	EditorPluginList();
	~EditorPluginList();
};

struct EditorProgressBG {
	String task;
	void step(int p_step = -1) { EditorNode::progress_task_step_bg(task, p_step); }
	EditorProgressBG(const String &p_task, const String &p_label, int p_amount) {
		EditorNode::progress_add_task_bg(p_task, p_label, p_amount);
		task = p_task;
	}
	~EditorProgressBG() { EditorNode::progress_end_task_bg(task); }
};

#endif // EDITOR_NODE_H
