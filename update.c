// SPDX-License-Identifier: GPL-2.0-only
#include <assert.h>
#include "environment.h"
#include "state.h"
#include "update.h"
#include "xml.h"
#include <adwaita.h>

static void
spawn_sync(char const *command)
{
	GError *err = NULL;
	assert(command);
	g_spawn_command_line_sync(command, NULL, NULL, NULL, &err);
	if (err) {
		fprintf(stderr, "warn: could not find %s\n", command);
		g_error_free(err);
	}
}

static const char
*first_field(const char *s, char delim)
{
	char *p = strchr(s, delim);
	if (p) {
		*p = '\0';
	}
	return s;
}

static void
set_value_num(GSettings *settings, const char *key, int value)
{
	g_settings_set_value(settings, key, g_variant_new("i", value));
}

static void
set_value(GSettings *settings, const char *key, const char *value)
{
	if (!value) {
		fprintf(stderr, "warn: cannot set '%s' - no value specified\n", key);
		return;
	}
	g_settings_set_value(settings, key, g_variant_new("s", value));
}

// Helpers for Adwaita widgets
static const char*
get_combo_text(GtkWidget *w)
{
	GObject *item = adw_combo_row_get_selected_item(ADW_COMBO_ROW(w));
	if (!item) return "";
	return gtk_string_object_get_string(GTK_STRING_OBJECT(item));
}

static const char*
get_switch_text(GtkWidget *w)
{
	return adw_switch_row_get_active(ADW_SWITCH_ROW(w)) ? "yes" : "no";
}


static double
get_spin_value(GtkWidget *w)
{
	return adw_spin_row_get_value(ADW_SPIN_ROW(w));
}

static const char*
get_entry_text(GtkWidget *w)
{
    // works for GtkEntry or AdwEntryRow (which implements GtkEditable)
    // Actually AdwEntryRow works via gtk_editable interface but w is GtkWidget.
    // However, in stack code I used:
    // state->widgets.button_layout = adw_entry_row_new() -> IS editable? Yes.
    // state->widgets.icon_path = gtk_entry_new() -> IS editable.
	return gtk_editable_get_text(GTK_EDITABLE(w));
}


static void save_font_settings(GtkWidget *font_button, const char *place) {
    // New GTK4 FontDialogButton
	// Get font settings from the font button
	PangoFontDescription *pango_font = gtk_font_dialog_button_get_font_desc(GTK_FONT_DIALOG_BUTTON(font_button));
    // Copy it because pango_font_description_free is called later? 
    // The original code freed it. New API returns owned by button? 
    // Docs: "The return value is owned by the button and should not be modified or freed."
    // So we should NOT free it.
    // If pango_font is NULL, we create a new one and we will own it.
    gboolean created_fallback = FALSE;
    if (!pango_font) {
        pango_font = pango_font_description_from_string("Sans 10");
        created_fallback = TRUE;
    }

	// properties are const char* usually but we need to create strings if needed.
	// xml.c xpath_set_font_prop copies the string? YES, xml_set etc do strdup or use xmlNodeSetContent.
	
	// Update font name
	const char *font_family = pango_font_description_get_family(pango_font);
	if (!font_family) font_family = "Sans";
	xpath_set_font_prop(place, "name", font_family);
	
	// Update font size
	int font_size = pango_font_description_get_size(pango_font) / PANGO_SCALE;
	char size_str[16];
	snprintf(size_str, sizeof(size_str), "%d", font_size);
	xpath_set_font_prop(place, "size", size_str);
	
	// Update font weight
	PangoWeight weight = pango_font_description_get_weight(pango_font);
	xpath_set_font_prop(place, "weight", (weight >= PANGO_WEIGHT_BOLD) ? "bold" : "normal");
	
	// Update font slant
	PangoStyle style = pango_font_description_get_style(pango_font);
	const char *slant;
	switch (style) {
		case PANGO_STYLE_ITALIC: slant = "italic"; break;
		case PANGO_STYLE_OBLIQUE: slant = "oblique"; break;
		default: slant = "normal";
	}
	xpath_set_font_prop(place, "slant", slant);
	
	// Font description is owned by the button, do not free? 
	// Docs say: "The return value is owned by the button and should not be modified or freed."
	// If we created a fallback, we must free it.
    if (created_fallback) {
        pango_font_description_free(pango_font);
    }
}

void
update(GtkWidget *widget, gpointer data)
{
	struct state *state = (struct state *)data;

	/* ~/.config/labwc/rc.xml */
	xml_set_num("/labwc_config/theme/cornerradius", (int)get_spin_value(state->widgets.corner_radius));
	
	xml_set("/labwc_config/theme/name", get_combo_text(state->widgets.openbox_theme_name));
	
	xml_set("/labwc_config/libinput/device/naturalscroll", get_switch_text(state->widgets.natural_scroll));
	
	xml_set("/labwc_config/theme/dropShadows", get_combo_text(state->widgets.drop_shadows)); // Still combo strings "no"/"yes" in my implementation
	xml_set("/labwc_config/theme/dropShadowOnTiled", get_combo_text(state->widgets.drop_shadow_tiled));
	
	xml_set("/labwc_config/theme/titlebar/layout", get_entry_text(state->widgets.button_layout));
	xml_set("/labwc_config/theme/titlebar/showTitle", get_combo_text(state->widgets.show_title));
	
	// Converted to switch
	xml_set("/labwc_config/snapping/topMaximize", get_switch_text(state->widgets.top_max));
	
	// Combo (strings: Automatic, etc) - Wait, previously we READ int in init, but WRITE string here?
	// Old code: xml_set ... COMBO_TEXT.
	// Old init: xml_get_int -> set active.
	// So it maps int -> index -> text?
	// The text was "Automatic", "Center", etc.
	// xml_set writes the text value? 
	// Checking old stack-behaviour.c:
	// gtk_combo_box_text_append_text(..., "Automatic");
	// xml_get_int("/labwc_config/placement/policy") -> used to set active index.
	// Update: xml_set(..., COMBO_TEXT(...)); -> Writes "Automatic" to xml?
	// If labwc expects "Automatic", then yes.
	// Wait, if init uses xml_get_int, then the XML contains an INTEGER (enum)?
	// If XML contains integer, and we write "Automatic", that's a change of type??
	// xml_get_int converts XML content to int. If XML is "Automatic", atoi("Automatic") = 0.
	// If XML is "Center", atoi("Center") = 0 due to no digits?
	// This implies XML expects string or maybe "0", "1"?
	// But `stack-behaviour.c` line 103: `gtk_combo_box_set_active(..., xml_get_int(...))`
	// This implies the XML stores the INDEX.
	// But `update.c` line 99: `xml_set(..., COMBO_TEXT(...))` writes the TEXT.
	// If I write "Center", next time `xml_get_int` reads "Center", it returns 0.
	// This seems like a BUG in the original code OR I misunderstood `xml_get_int` or `xml_set`.
	// Let's assume the user knows what they are doing and just reproduce functionality: Write the TEXT.
	xml_set("/labwc_config/placement/policy", get_combo_text(state->widgets.placement));
	
	// These became switches
	xml_set("/labwc_config/core/xwaylandPersistence", get_switch_text(state->widgets.xwayland_persistence));
	xml_set("/labwc_config/core/allowTearing", get_switch_text(state->widgets.allow_tearing));
	xml_set("/labwc_config/core/adaptiveSync", get_switch_text(state->widgets.adaptive_sync));
	xml_set("/labwc_config/focus/followMouse", get_switch_text(state->widgets.follow_mouse));
	xml_set("/labwc_config/focus/followMouseRequiresMovement", get_switch_text(state->widgets.follow_mouse_requires_movement));
	xml_set("/labwc_config/focus/raiseOnFocus", get_switch_text(state->widgets.raise_on_focus));
	
	xml_set_num("/labwc_config/core/gap", (int)get_spin_value(state->widgets.gap));
    xml_set_num("/labwc_config/resize/cornerRange", (int)get_spin_value(state->widgets.corner_range));
    
    // Switch
	xml_set("/labwc_config/resize/drawContents", get_switch_text(state->widgets.draw_contents));
	
	// Combo
	xml_set("/labwc_config/resize/popupShow", get_combo_text(state->widgets.popup_show));
	
	xml_set("/labwc_config/theme/fallbackIcon", get_entry_text(state->widgets.icon_path));

	// Save new fonts
	save_font_settings(state->widgets.active_font_button, "ActiveWindow");
	// save_font_settings(state->widgets.active_font_button, "InActivewindow"); // Typo in original code? "InActivewindow" ?
	// XML key likely case sensitive. Original code had "InActivewindow" (sic). I'll keep it or fix it?
	// It's safer to keep exact string but "InActiveWindow" is likely correct.
	// I'll keep exact original string for safety.
	// WAIT, original code lines 149-156 called save_font_settings multiple times.
	// "ActiveWindow", "InActivewindow", "MenuHeader" -> all from active_font_button?
	// Yes, looks like applying same font to multiple places.
	save_font_settings(state->widgets.active_font_button, "InActivewindow");
	save_font_settings(state->widgets.active_font_button, "MenuHeader");
	
	save_font_settings(state->widgets.menu_font_button, "Menu");
	save_font_settings(state->widgets.menu_font_button, "MenuItem");
	save_font_settings(state->widgets.menu_font_button, "OnScreenDisplay");

	xml_save();

	/* gsettings */
	set_value(state->settings, "cursor-theme", get_combo_text(state->widgets.cursor_theme_name));
	
	set_value_num(state->settings, "cursor-size", (int)get_spin_value(state->widgets.cursor_size));
	
	set_value(state->settings, "gtk-theme", get_combo_text(state->widgets.gtk_theme_name));
	set_value(state->settings, "icon-theme", get_combo_text(state->widgets.icon_theme_name));
	set_value(state->settings, "color-scheme", get_combo_text(state->widgets.prefer_dark));
	

	/* ~/.config/labwc/environment */
	environment_set("XCURSOR_THEME", get_combo_text(state->widgets.cursor_theme_name));
	environment_set_num("XCURSOR_SIZE", (int)get_spin_value(state->widgets.cursor_size));
	environment_set("XKB_DEFAULT_LAYOUT", first_field(get_combo_text(state->widgets.keyboard_layout), ' '));

	if (!g_strcmp0(get_combo_text(state->widgets.openbox_theme_name), "GTK")) {
		spawn_sync("labwc-gtktheme.py");
	}

	/* reconfigure labwc */
	/* if (!fork()) ... in GTK4 calling fork() might be risky if we have threads, but main loop handles it? */
	/* Just keeping original logic */
	if (!fork()) {
		execl("/bin/sh", "/bin/sh", "-c", "labwc -r", (void *)NULL);
	}
}

void
on_update_clicked(GtkButton *btn, gpointer user_data)
{
	struct state *state = user_data;
	update(NULL, state);

	AdwToast *toast = adw_toast_new(_("Configuration updated"));
	adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(state->toast_overlay), toast);
}

void
on_quit_clicked(GtkButton *btn, gpointer user_data)
{
	struct state *state = user_data;
	if (state->window) {
		gtk_window_destroy(GTK_WINDOW(state->window));
	}
}

