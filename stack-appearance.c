// SPDX-License-Identifier: GPL-2.0-only
#include "keyboard-layouts.h"
#include "state.h"
#include "stack-appearance.h"
#include "theme.h"
#include "xml.h"
#include "update.h"
#include <adwaita.h>

static void on_font_set(GtkFontDialogButton *button, struct state *state);
static void setup_font_button(GtkFontDialogButton *button, const char *place);

static void
on_font_set(GtkFontDialogButton *button, struct state *state)
{
	// Just trigger the general update function
	update(NULL, state);
}

static void
append_combo_row(AdwPreferencesGroup *group, const char *title, const char *xml_path, struct state *state, GtkWidget **widget_ptr, struct themes *themes)
{
	GtkWidget *row = adw_combo_row_new();
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
	adw_preferences_group_add(group, row);
	if (widget_ptr) *widget_ptr = row;

	const char *active_id = xml_get(xml_path);
	GtkStringList *string_list = gtk_string_list_new(NULL);
	int active_index = 0;
	
	if (themes) {
		for (int i = 0; i < themes->nr; ++i) {
			gtk_string_list_append(string_list, themes->data[i].name);
			if (active_id && !strcmp(themes->data[i].name, active_id)) {
				active_index = i;
			}
		}
	}

	adw_combo_row_set_model(ADW_COMBO_ROW(row), G_LIST_MODEL(string_list));
	adw_combo_row_set_selected(ADW_COMBO_ROW(row), active_index);
}


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

void
stack_appearance_init(struct state *state, GtkWidget *stack)
{
	GtkWidget *page = adw_preferences_page_new();
	adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(page), _("Appearance"));
	adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(page), "preferences-desktop-appearance-symbolic");
	adw_view_stack_add_titled(ADW_VIEW_STACK(stack), page, "appearance", _("Appearance"));

	GtkWidget *group = adw_preferences_group_new();
	adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), _("Theme"));
	adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

	/* Openbox Theme */
	struct themes openbox_themes = { 0 };
	theme_find(&openbox_themes, "themes", "openbox-3/themerc");
	append_combo_row(ADW_PREFERENCES_GROUP(group), _("Openbox Theme"), "/labwc_config/theme/name", state, &state->widgets.openbox_theme_name, &openbox_themes);
	theme_free_vector(&openbox_themes);

	/* GTK Theme */
	// We read current GTK theme from GSettings, not XML
	struct themes gtk_themes = { 0 };
	theme_find(&gtk_themes, "themes", "gtk-4.0/gtk.css"); // Tweaking for gtk4? or keep gtk-3.0 path? Sticking to logic.
	// Actually theme.c finds paths. Let's assume standard paths.
	// NOTE: GTK3 themes might not work perfectly on GTK4 but let's list them or look for gtk-4.0.
	// Retaining original logic path suffix: "gtk-3.0/gtk.css" -> maybe "gtk-4.0/gtk.css" is better now?
	// Let's stick with what was there or improve. "gtk-3.0" implies GTK3. "gtk-4.0" is modern.
	// Let's use "gtk-4.0/gtk.css" for safety if we are a gtk4 app.
	theme_find(&gtk_themes, "themes", "gtk-4.0/gtk.css");
	
	GtkStringList *gtk_list = gtk_string_list_new(NULL);
	char *active_gtk_id = g_settings_get_string(state->settings, "gtk-theme");
	int active_gtk_idx = 0;
	// Always append "Default" or similar?
	for (int i = 0; i < gtk_themes.nr; ++i) {
		gtk_string_list_append(gtk_list, gtk_themes.data[i].name);
		if (active_gtk_id && !strcmp(gtk_themes.data[i].name, active_gtk_id)) {
			active_gtk_idx = i;
		}
	}
	// Fallback to gtk-3.0 search if empty? No, let's just use what we found.
	append_combo_row_full(ADW_PREFERENCES_GROUP(group), _("GTK Theme"), state, &state->widgets.gtk_theme_name, gtk_list, active_gtk_idx);
	theme_free_vector(&gtk_themes);

	/* Icon Theme */
	struct themes icon_themes = { 0 };
	theme_find(&icon_themes, "icons", NULL);
	GtkStringList *icon_list = gtk_string_list_new(NULL);
	char *active_icon_id = g_settings_get_string(state->settings, "icon-theme");
	int active_icon_idx = 0;
	for (int i = 0; i < icon_themes.nr; ++i) {
		gtk_string_list_append(icon_list, icon_themes.data[i].name);
		if (active_icon_id && !strcmp(icon_themes.data[i].name, active_icon_id)) {
			active_icon_idx = i;
		}
	}
	append_combo_row_full(ADW_PREFERENCES_GROUP(group), _("Icon Theme"), state, &state->widgets.icon_theme_name, icon_list, active_icon_idx);
	theme_free_vector(&icon_themes);
	
	/* Color Scheme */
	GtkStringList *color_list = gtk_string_list_new(NULL);
	gtk_string_list_append(color_list, "default");
	gtk_string_list_append(color_list, "prefer-dark");
	gtk_string_list_append(color_list, "prefer-light");
	// enum val: 0=default, 1=prefer-dark, 2=prefer-light
	int color_val = g_settings_get_enum(state->settings, "color-scheme");
	append_combo_row_full(ADW_PREFERENCES_GROUP(group), _("Color Scheme"), state, &state->widgets.prefer_dark, color_list, color_val);


	// Font Group
	GtkWidget *font_group = adw_preferences_group_new();
	adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(font_group), _("Fonts"));
	adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(font_group));

	// Active Window Font
	state->widgets.active_font_button = gtk_font_dialog_button_new(gtk_font_dialog_new());
	setup_font_button(GTK_FONT_DIALOG_BUTTON(state->widgets.active_font_button), "ActiveWindow");
	g_signal_connect(state->widgets.active_font_button, "notify::font-desc", G_CALLBACK(on_font_set), state);

	GtkWidget *ft_row = adw_action_row_new();
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ft_row), _("Active Window Font"));
	adw_action_row_add_suffix(ADW_ACTION_ROW(ft_row), state->widgets.active_font_button);
	adw_preferences_group_add(ADW_PREFERENCES_GROUP(font_group), ft_row);

	// Menu Font
	state->widgets.menu_font_button = gtk_font_dialog_button_new(gtk_font_dialog_new());
	setup_font_button(GTK_FONT_DIALOG_BUTTON(state->widgets.menu_font_button), "Menu");
	g_signal_connect(state->widgets.menu_font_button, "notify::font-desc", G_CALLBACK(on_font_set), state);

	GtkWidget *mf_row = adw_action_row_new();
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mf_row), _("Menu Font"));
	adw_action_row_add_suffix(ADW_ACTION_ROW(mf_row), state->widgets.menu_font_button);
	adw_preferences_group_add(ADW_PREFERENCES_GROUP(font_group), mf_row);

	// Decoration Group
	GtkWidget *deco_group = adw_preferences_group_new();
	adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(deco_group), _("Decoration"));
	adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(deco_group));

	// Corner Radius
	state->widgets.corner_radius = adw_spin_row_new(gtk_adjustment_new(0, 0, 64, 1, 0, 0), 1.0, 0);
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(state->widgets.corner_radius), _("Corner Radius"));
	adw_spin_row_set_value(ADW_SPIN_ROW(state->widgets.corner_radius), (double)xml_get_int("/labwc_config/theme/cornerradius"));
	adw_preferences_group_add(ADW_PREFERENCES_GROUP(deco_group), state->widgets.corner_radius);

	// Button Layout
	state->widgets.button_layout = adw_entry_row_new();
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(state->widgets.button_layout), _("Button Layout"));
	gtk_editable_set_text(GTK_EDITABLE(state->widgets.button_layout), xml_get("/labwc_config/theme/titlebar/layout"));
	gtk_widget_set_tooltip_text(state->widgets.button_layout, "Values: icon,menu,shade:max,iconify,close");
	
	adw_preferences_group_add(ADW_PREFERENCES_GROUP(deco_group), state->widgets.button_layout);

	// Show Title
	GtkStringList *bool_list = gtk_string_list_new(NULL);
	gtk_string_list_append(bool_list, "no");
	gtk_string_list_append(bool_list, "yes");
	int show_title_val = xml_get_bool_text("/labwc_config/theme/titlebar/showTitle"); // 0 or 1
	append_combo_row_full(ADW_PREFERENCES_GROUP(deco_group), _("Show Title"), state, &state->widgets.show_title, bool_list, show_title_val);

	// Drop Shadows
	GtkStringList *ds_list = gtk_string_list_new(NULL);
	gtk_string_list_append(ds_list, "no");
	gtk_string_list_append(ds_list, "yes");
	int ds_val = xml_get_bool_text("/labwc_config/theme/dropShadows");
	append_combo_row_full(ADW_PREFERENCES_GROUP(deco_group), _("Drop Shadows"), state, &state->widgets.drop_shadows, ds_list, ds_val);

	// Drop Shadows Tiled
	GtkStringList *dst_list = gtk_string_list_new(NULL);
	gtk_string_list_append(dst_list, "no");
	gtk_string_list_append(dst_list, "yes");
	int dst_val = xml_get_bool_text("/labwc_config/theme/dropShadowOnTiled");
	append_combo_row_full(ADW_PREFERENCES_GROUP(deco_group), _("Drop Shadow on Tiled"), state, &state->widgets.drop_shadow_tiled, dst_list, dst_val);
}

static void setup_font_button(GtkFontDialogButton *button, const char *place) 
{
	char *font_name = xpath_get_font_prop(place, "name");
	char *font_size = xpath_get_font_prop(place, "size");
	char *font_weight = xpath_get_font_prop(place, "weight");
	char *font_slant = xpath_get_font_prop(place, "slant");
	
	if (font_name) {
		char font_string[256];
		snprintf(font_string, sizeof(font_string), "%s %s %s %s", 
			font_name,
			font_weight ? font_weight : "",
			font_slant && strcmp(font_slant, "normal") ? font_slant : "",
			font_size ? font_size : "10");
			
		PangoFontDescription *desc = pango_font_description_from_string(font_string);
		gtk_font_dialog_button_set_font_desc(button, desc);
		pango_font_description_free(desc);
	} else {
		PangoFontDescription *desc = pango_font_description_from_string("Sans 10");
		gtk_font_dialog_button_set_font_desc(button, desc);
		pango_font_description_free(desc);
	}
	
	if (font_name) free(font_name);
	if (font_size) free(font_size);
	if (font_weight) free(font_weight);
	if (font_slant) free(font_slant);
}

