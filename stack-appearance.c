// SPDX-License-Identifier: GPL-2.0-only
#include "keyboard-layouts.h"
#include "state.h"
#include "stack-appearance.h"
#include "theme.h"
#include "xml.h"
static void update_preview(const char *filename, GtkWidget *preview_image) {
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(filename, 64, 64, NULL);
    if (pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(preview_image), pixbuf);
        g_object_unref(pixbuf);
    } else {
        gtk_image_clear(GTK_IMAGE(preview_image));
    }
}

static void on_button_clicked(GtkWidget *button, gpointer user_data)
{
    struct state *state = user_data;
    GtkWidget *dialog;
    GtkFileFilter *filter;
    
    dialog = gtk_file_chooser_dialog_new("Select Icon",
                                        GTK_WINDOW(gtk_widget_get_toplevel(button)),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Open", GTK_RESPONSE_ACCEPT,
                                        NULL);

    // Create and set up the file filter
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image Files");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/svg+xml");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.svg");
    
    // Add the filter to the dialog
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        // Store the filename in the state widget for the update function
        gtk_entry_set_text(GTK_ENTRY(state->widgets.icon_path), filename);
        // Update the XML immediately
        xml_set("/labwc_config/theme/fallbackIcon", filename);
        xml_save();
        // Update the preview
        update_preview(filename, state->widgets.icon_preview);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void
stack_appearance_init(struct state *state, GtkWidget *stack)
{
	GtkWidget *widget;

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_stack_add_named(GTK_STACK(stack), vbox, "appearance");
	gtk_container_child_set(GTK_CONTAINER(stack), vbox, "title", _("Appearance"), NULL);

	/* the grid with settings */
	int row = 0;
	GtkWidget *grid = gtk_grid_new();
	g_object_set(grid, "margin", 20, "row-spacing", 10, "column-spacing", 10, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 5);

widget = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Labwc Options</b>");
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row++, 2, 1);
 
 	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row++, 2, 1);

	/* openbox theme combobox */
	struct themes openbox_themes = { 0 };
	theme_find(&openbox_themes, "themes", "openbox-3/themerc");
	widget = gtk_label_new(_("Labwc Theme"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.openbox_theme_name = gtk_combo_box_text_new();
	int active = -1;
	char *active_id = xml_get("/labwc_config/theme/name");
	struct theme *theme;
	for (int i = 0; i < openbox_themes.nr; ++i) {
		theme = openbox_themes.data + i;
		if (active_id && !strcmp(theme->name, active_id)) {
			active = i;
		}
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.openbox_theme_name), theme->name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.openbox_theme_name), active);
	gtk_grid_attach(GTK_GRID(grid), state->widgets.openbox_theme_name, 1, row++, 1, 1);
	theme_free_vector(&openbox_themes);

	/* corner radius spinbutton */
	widget = gtk_label_new(_("Corner Radius"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	GtkAdjustment *adjustment = gtk_adjustment_new(0, 0, 20, 1, 2, 0);
	state->widgets.corner_radius = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->widgets.corner_radius), xml_get_int("/labwc_config/theme/cornerradius"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.corner_radius, 1, row++, 1, 1);

        /* button layout */
	widget = gtk_label_new(_("Button Layout"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.button_layout = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(state->widgets.button_layout), xml_get("/labwc_config/theme/titlebar/layout"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.button_layout , 1, row++, 1, 1);
        state->widgets.label = gtk_label_new ("Values: icon,menu,shade:max,iconify,close" );
	gtk_grid_attach(GTK_GRID(grid), state->widgets.label, 1, row++, 1, 1);

        /* show title? */
	widget = gtk_label_new(_("Show Title"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.show_title = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.show_title), "no");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.show_title), "yes");
        gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.show_title), xml_get_bool_text("/labwc_config/theme/titlebar/showTitle"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.show_title, 1, row++, 1, 1);


	/* drop shadows */
	widget = gtk_label_new(_("Drop Shadows"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.drop_shadows = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.drop_shadows), "no");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.drop_shadows), "yes");
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.drop_shadows), xml_get_bool_text("/labwc_config/theme/dropShadows"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.drop_shadows, 1, row++, 1, 1);	


 /* show title? */
	widget = gtk_label_new(_("Show Menu Icons"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.show_icons = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.show_icons), "no");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.show_icons), "yes");
        gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.show_icons), xml_get_bool_text("/labwc_config/menu/showIcons"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.show_icons, 1, row++, 1, 1);


/* File chooser button and preview */
	widget = gtk_label_new(_("Select FallBack Icon"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

	// Create a horizontal box for the button and preview
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	
	state->widgets.file_button = gtk_button_new_with_label("Select Icon File");
	g_signal_connect(state->widgets.file_button, "clicked", G_CALLBACK(on_button_clicked), state);
	gtk_box_pack_start(GTK_BOX(hbox), state->widgets.file_button, FALSE, FALSE, 0);

	// Add preview image
	state->widgets.icon_preview = gtk_image_new();
	gtk_image_set_pixel_size(GTK_IMAGE(state->widgets.icon_preview), 64);
	gtk_box_pack_start(GTK_BOX(hbox), state->widgets.icon_preview, FALSE, FALSE, 5);

	// Add the hidden entry for storing the path
	state->widgets.icon_path = gtk_entry_new();
	gtk_widget_set_no_show_all(state->widgets.icon_path, TRUE);
	gtk_widget_hide(state->widgets.icon_path);
	gtk_box_pack_start(GTK_BOX(hbox), state->widgets.icon_path, FALSE, FALSE, 0);

	gtk_grid_attach(GTK_GRID(grid), hbox, 1, row++, 1, 1);

	// Load initial preview if there's a saved icon path
	const char *initial_path = xml_get("/labwc_config/theme/fallbackIcon");
	if (initial_path) {
		gtk_entry_set_text(GTK_ENTRY(state->widgets.icon_path), initial_path);
		update_preview(initial_path, state->widgets.icon_preview);
	}
	



widget = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(widget), "<b>Other Theme and Style Options</b>");
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row++, 2, 1);
 
 	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row++, 2, 1);




	/* gtk theme combobox */
	struct themes gtk_themes = { 0 };
	theme_find(&gtk_themes, "themes", "gtk-3.0/gtk.css");

	widget = gtk_label_new(_("Gtk Theme"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.gtk_theme_name = gtk_combo_box_text_new();

	active_id = g_settings_get_string(state->settings, "gtk-theme");
	active = -1;
	for (int i = 0; i < gtk_themes.nr; ++i) {
		theme = gtk_themes.data + i;
		if (!strcmp(theme->name, active_id)) {
			active = i;
		}
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.gtk_theme_name), theme->name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.gtk_theme_name), active);
	gtk_grid_attach(GTK_GRID(grid), state->widgets.gtk_theme_name, 1, row++, 1, 1);
	theme_free_vector(&gtk_themes);

	/* Color Scheme - not sure how well this works outside gnome? */
	
	widget = gtk_label_new(_("Color Scheme"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.prefer_dark = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.prefer_dark), "default");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.prefer_dark), "prefer-dark");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.prefer_dark), "prefer-light");
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.prefer_dark), g_settings_get_enum(state->settings, "color-scheme"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.prefer_dark, 1, row++, 1, 1);


	/* icon theme combobox */
	struct themes icon_themes = { 0 };
	theme_find(&icon_themes, "icons", NULL);

	widget = gtk_label_new(_("Icon Theme"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.icon_theme_name = gtk_combo_box_text_new();

	active_id = g_settings_get_string(state->settings, "icon-theme");
	active = -1;
	for (int i = 0; i < icon_themes.nr; ++i) {
		theme = icon_themes.data + i;
		if (!strcmp(theme->name, active_id)) {
			active = i;
		}
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.icon_theme_name), theme->name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.icon_theme_name), active);
	gtk_grid_attach(GTK_GRID(grid), state->widgets.icon_theme_name, 1, row++, 1, 1);
	theme_free_vector(&icon_themes);
	
	
}

