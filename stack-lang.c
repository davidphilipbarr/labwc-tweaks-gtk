// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include "environment.h"
#include "keyboard-layouts.h"
#include "state.h"
#include "stack-lang.h"
#include "theme.h"
#include "xml.h"
#include <adwaita.h>

void
stack_lang_init(struct state *state, GtkWidget *stack)
{
	GtkWidget *page = adw_preferences_page_new();
	adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), _("Language & Region"));
	adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page), "preferences-desktop-keyboard-symbolic");
	adw_view_stack_add_titled(ADW_VIEW_STACK(stack), page, "language", _("Language"));

	GtkWidget *group = adw_preferences_group_new();
	adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), _("Keyboard"));
	adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

	/* keyboard layout */
	GList *keyboard_layouts = NULL;
	keyboard_layouts_init(&keyboard_layouts, "/usr/share/X11/xkb/rules/evdev.lst");

	state->widgets.keyboard_layout = adw_combo_row_new();
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(state->widgets.keyboard_layout), _("Keyboard Layout"));
	adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), state->widgets.keyboard_layout);

	char xkb_default_layout[1024];
	environment_get(xkb_default_layout, sizeof(xkb_default_layout), "XKB_DEFAULT_LAYOUT");
	int active = 0;

	GtkStringList *layout_list = gtk_string_list_new(NULL);
	GList *iter;
	int i = 0;
	// We might have many layouts, building string list is fine.
	for (iter = keyboard_layouts; iter; iter = iter->next) {
		struct layout *layout = (struct layout *)iter->data;
		if (!strcmp(layout->lang, xkb_default_layout)) {
			active = i;
		}
		char buf[256];
		snprintf(buf, sizeof(buf), "%s  %s", layout->lang, layout->description);
		gtk_string_list_append(layout_list, buf);
		++i;
	}
	adw_combo_row_set_model(ADW_COMBO_ROW(state->widgets.keyboard_layout), G_LIST_MODEL(layout_list));
	adw_combo_row_set_selected(ADW_COMBO_ROW(state->widgets.keyboard_layout), active);

	keyboard_layouts_finish(keyboard_layouts);
}

