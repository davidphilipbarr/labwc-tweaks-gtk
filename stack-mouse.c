// SPDX-License-Identifier: GPL-2.0-only
#include "keyboard-layouts.h"
#include "state.h"
#include "stack-mouse.h"
#include "theme.h"
#include "xml.h"
#include <adwaita.h>

static void
append_combo_row_full(AdwPreferencesGroup *group, const char *title, struct state *state, GtkWidget **widget_ptr, GtkStringList *list, int active_idx)
{
	GtkWidget *row = adw_combo_row_new();
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
	adw_preferences_group_add(group, row);
	if (widget_ptr) *widget_ptr = row;

	adw_combo_row_set_model(ADW_COMBO_ROW(row), G_LIST_MODEL(list));
	adw_combo_row_set_selected(ADW_COMBO_ROW(row), active_idx);
}

static GtkWidget*
create_switch_row(AdwPreferencesGroup *group, const char *title, const char *xml_path, GtkWidget **widget_ptr)
{
	GtkWidget *row = adw_switch_row_new();
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
	adw_switch_row_set_active(ADW_SWITCH_ROW(row), xml_get_bool_text(xml_path));
	adw_preferences_group_add(group, row);
	if (widget_ptr) *widget_ptr = row;
	return row;
}


void
stack_mouse_init(struct state *state, GtkWidget *stack)
{
	GtkWidget *page = adw_preferences_page_new();
	adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), _("Mouse & Touchpad"));
	adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page), "input-mouse-symbolic"); // or preferences-desktop-peripherals
	adw_view_stack_add_titled(ADW_VIEW_STACK(stack), page, "mouse", _("Mouse"));
	
	GtkWidget *group = adw_preferences_group_new();
	adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), _("Cursor"));
	adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

	/* Cursor Theme */
	struct themes cursor_themes = { 0 };
	theme_find(&cursor_themes, "icons", "cursors");
	GtkStringList *theme_list = gtk_string_list_new(NULL);
	char *active_id = g_settings_get_string(state->settings, "cursor-theme");
	int active = 0;
	if (cursor_themes.nr == 0) {
		// fallback?
		gtk_string_list_append(theme_list, "Default");
	}
	for (int i = 0; i < cursor_themes.nr; ++i) {
		struct theme *theme = cursor_themes.data + i;
		gtk_string_list_append(theme_list, theme->name);
		if (active_id && !strcmp(theme->name, active_id)) {
			active = i;
		}
	}
	append_combo_row_full(ADW_PREFERENCES_GROUP(group), _("Cursor Theme"), state, &state->widgets.cursor_theme_name, theme_list, active);
	theme_free_vector(&cursor_themes);

	/* Cursor Size */
	state->widgets.cursor_size = adw_spin_row_new(gtk_adjustment_new(0, 0, 512, 1, 0, 0), 1.0, 0);
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(state->widgets.cursor_size), _("Cursor Size"));
	// g_settings_get_int returns int, AdwSpinRow expects double
	adw_spin_row_set_value(ADW_SPIN_ROW(state->widgets.cursor_size), (double)g_settings_get_int(state->settings, "cursor-size"));
	adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), state->widgets.cursor_size);

	/* Touchpad */
	GtkWidget *touch_group = adw_preferences_group_new();
	adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(touch_group), _("Touchpad"));
	adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(touch_group));

	/* Natural Scroll */
	create_switch_row(ADW_PREFERENCES_GROUP(touch_group), _("Natural Scroll"), "/labwc_config/libinput/device/naturalscroll", &state->widgets.natural_scroll);
}

