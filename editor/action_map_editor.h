/*************************************************************************/
/*  action_map_editor.h                                                  */
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

#ifndef ACTION_MAP_EDITOR_H
#define ACTION_MAP_EDITOR_H

#include "editor/editor_data.h"

// Confirmation Dialog used when configuring an input event.
// Separate from ActionMapEditor for code cleanliness and separation of responsibilities.
class InputEventConfigurationDialog : public ConfirmationDialog {
	GDCLASS(InputEventConfigurationDialog, ConfirmationDialog);

public:
	enum InputType {
		INPUT_KEY = 1,
		INPUT_MOUSE_BUTTON = 2,
		INPUT_JOY_BUTTON = 4,
		INPUT_JOY_MOTION = 8
	};

private:
	struct IconCache {
		Ref<Texture2D> keyboard;
		Ref<Texture2D> mouse;
		Ref<Texture2D> joypad_button;
		Ref<Texture2D> joypad_axis;
	} icon_cache;

	Ref<InputEvent> event = Ref<InputEvent>();

	// Listening for input

	Button *listen_button;
	Label *event_as_text;

	// List of All Key/Mouse/Joypad input options.

	int allowed_input_types;
	Tree *input_list_tree;
	LineEdit *input_list_search;

	// Additional Options, shown depending on event selected

	VBoxContainer *additional_options_container;

	HBoxContainer *device_container;
	OptionButton *device_id_option;

	String mods[5] = { "Alt", "Shift", "Command", "Control", "Meta" };
	HBoxContainer *mod_container;
	CheckBox *mod_checkboxes[5];
	CheckBox *store_command_checkbox;

	CheckBox *physical_key_checkbox;

	void _set_event(const Ref<InputEvent> &p_event);

	void _listen_toggled(bool p_pressed);
	void _listen_window_input(const Ref<InputEvent> &p_event);

	void _search_term_updated(const String &p_term);
	void _update_input_list();
	void _input_list_item_selected();

	void _mod_toggled(bool p_checked, int p_index);
	void _store_command_toggled(bool p_checked);
	void _physical_keycode_toggled(bool p_checked);

	void _set_current_device(int i_device);
	int _get_current_device() const;
	String _get_device_string(int i_device) const;

protected:
	void _notification(int p_what);

public:
	// Pass an existing event to configure it. Alternatively, pass no event to start with a blank configuration.
	void popup_and_configure(const Ref<InputEvent> &p_event = Ref<InputEvent>());
	Ref<InputEvent> get_event() const;

	void set_allowed_input_types(int p_type_masks);

	InputEventConfigurationDialog();
};

class ActionMapEditor : public Control {
	GDCLASS(ActionMapEditor, Control);

public:
	struct ActionInfo {
		String name = String();
		Dictionary action = Dictionary();

		Ref<Texture2D> icon = Ref<Texture2D>();
		bool editable = true;
	};

private:
	enum ItemButton {
		BUTTON_ADD_EVENT,
		BUTTON_EDIT_EVENT,
		BUTTON_REMOVE_ACTION,
		BUTTON_REMOVE_EVENT,
	};

	Vector<ActionInfo> actions_cache;
	Tree *action_tree;

	// Storing which action/event is currently being edited in the InputEventConfigurationDialog.

	Dictionary current_action = Dictionary();
	String current_action_name = String();
	int current_action_event_index = -1;

	// Popups

	InputEventConfigurationDialog *event_config_dialog;
	AcceptDialog *message;

	// Filtering and Adding actions

	bool show_uneditable;
	CheckBox *show_uneditable_actions_checkbox;
	LineEdit *action_list_search;

	bool allow_editing_actions;
	HBoxContainer *add_hbox;
	LineEdit *add_edit;

	void _event_config_confirmed();

	void _add_action_pressed();
	void _add_action(const String &p_name);
	void _action_edited();

	void _action_button_pressed(Object *p_item, int p_column, int p_id);
	void _search_term_updated(const String &p_search_term);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	LineEdit *get_search_box() const;
	InputEventConfigurationDialog *get_configuration_dialog();

	// Dictionary represents an Action with "events" (Array) and "deadzone" (float) items. Pass with no param to update list from cached action map.
	void update_action_list(const Vector<ActionInfo> &p_action_infos = Vector<ActionInfo>());
	void show_message(const String &p_message);

	void set_show_uneditable(bool p_show);
	void set_allow_editing_actions(bool p_allow);

	void set_toggle_editable_label(const String &p_label);

	ActionMapEditor();
};

#endif
