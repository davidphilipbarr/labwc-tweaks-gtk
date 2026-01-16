// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <gio/gio.h>
#include <adwaita.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include "state.h"
#include "stack-appearance.h"
#include "stack-behaviour.h"
#include "stack-lang.h"
#include "stack-mouse.h"
#include "update.h"
#include "xml.h"

static void
on_row_selected(GtkListBox *listbox, GtkListBoxRow *row, gpointer user_data)
{
	struct state *state = user_data;
	if (row) {
		const char *id = g_object_get_data(G_OBJECT(row), "page-id");
		adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(state->view_stack), id);
		adw_navigation_split_view_set_show_content(ADW_NAVIGATION_SPLIT_VIEW(state->split_view), TRUE);
	}
}

static void
add_sidebar_row(GtkListBox *listbox, const char *title, const char *id, const char *icon_name)
{
	GtkWidget *row = adw_action_row_new();
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
	g_object_set_data(G_OBJECT(row), "page-id", (gpointer)id);
	
	if (icon_name) {
		GtkWidget *image = gtk_image_new_from_icon_name(icon_name);
		adw_action_row_add_prefix(ADW_ACTION_ROW(row), image);
	}
	
	gtk_list_box_append(listbox, row);
}

static void
on_back_clicked(GtkWidget *btn, gpointer user_data)
{
	adw_navigation_split_view_set_show_content(ADW_NAVIGATION_SPLIT_VIEW(user_data), FALSE);
}

static void
activate(GtkApplication *app, gpointer user_data)
{
	struct state *state = (struct state *)user_data;

	/* window */
	state->window = (GtkWidget *)adw_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(state->window), _("Tweaks"));
	gtk_window_set_default_size(GTK_WINDOW(state->window), 900, 650);

	state->toast_overlay = adw_toast_overlay_new();
	adw_application_window_set_content(ADW_APPLICATION_WINDOW(state->window), state->toast_overlay);

	/* Split View */
	state->split_view = adw_navigation_split_view_new();
	adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(state->toast_overlay), state->split_view);
	adw_navigation_split_view_set_min_sidebar_width(ADW_NAVIGATION_SPLIT_VIEW(state->split_view), 250);

	/* Sidebar */
	GtkWidget *sidebar_list = gtk_list_box_new();
	gtk_widget_add_css_class(sidebar_list, "navigation-sidebar");
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(sidebar_list), GTK_SELECTION_SINGLE);

	GtkWidget *sidebar_scrolled = gtk_scrolled_window_new();
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sidebar_scrolled), sidebar_list);

	GtkWidget *sidebar_toolbar = adw_toolbar_view_new();
	GtkWidget *sidebar_header = adw_header_bar_new();
	adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(sidebar_toolbar), sidebar_header);
	adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(sidebar_toolbar), sidebar_scrolled);

	GtkWidget *sidebar_page = (GtkWidget *)adw_navigation_page_new(sidebar_toolbar, _("Settings"));
	adw_navigation_split_view_set_sidebar(ADW_NAVIGATION_SPLIT_VIEW(state->split_view), ADW_NAVIGATION_PAGE(sidebar_page));

	/* Content */
	state->view_stack = adw_view_stack_new();
	
	GtkWidget *content_toolbar = adw_toolbar_view_new();
	GtkWidget *content_header = adw_header_bar_new();
	adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(content_toolbar), content_header);
	adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(content_toolbar), state->view_stack);

	/* Apply & Quit in Content Header */
	GtkWidget *apply_btn = gtk_button_new_with_label(_("Apply"));
	gtk_widget_add_css_class(apply_btn, "suggested-action");
	g_signal_connect(apply_btn, "clicked", G_CALLBACK(on_update_clicked), state);
	adw_header_bar_pack_end(ADW_HEADER_BAR(content_header), apply_btn);

	GtkWidget *quit_btn = gtk_button_new_with_label(_("Quit"));
	g_signal_connect(quit_btn, "clicked", G_CALLBACK(on_quit_clicked), state);
	adw_header_bar_pack_end(ADW_HEADER_BAR(content_header), quit_btn);

	/* Navigation button for mobile */
	GtkWidget *nav_btn = gtk_button_new_from_icon_name("go-previous-symbolic");
	adw_header_bar_pack_start(ADW_HEADER_BAR(content_header), nav_btn);
	g_signal_connect(nav_btn, "clicked", G_CALLBACK(on_back_clicked), state->split_view);
	g_object_bind_property(state->split_view, "collapsed", nav_btn, "visible", G_BINDING_SYNC_CREATE);

	GtkWidget *content_page = (GtkWidget *)adw_navigation_page_new(content_toolbar, _("Details"));
	adw_navigation_split_view_set_content(ADW_NAVIGATION_SPLIT_VIEW(state->split_view), ADW_NAVIGATION_PAGE(content_page));

	/* stacks */
	stack_appearance_init(state, state->view_stack);
	stack_behaviour_init(state, state->view_stack);
	stack_mouse_init(state, state->view_stack);
	stack_lang_init(state, state->view_stack);

	/* Sidebar Rows */
	add_sidebar_row(GTK_LIST_BOX(sidebar_list), _("Appearance"), "appearance", "preferences-desktop-appearance-symbolic");
	add_sidebar_row(GTK_LIST_BOX(sidebar_list), _("Behaviour"), "behaviour", "focus-top-bar-symbolic");
	add_sidebar_row(GTK_LIST_BOX(sidebar_list), _("Mouse"), "mouse", "input-mouse-symbolic");
	add_sidebar_row(GTK_LIST_BOX(sidebar_list), _("Language"), "language", "preferences-desktop-keyboard-symbolic");

	g_signal_connect(sidebar_list, "row-selected", G_CALLBACK(on_row_selected), state);

	/* Select first row */
	GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(sidebar_list), 0);
	if (first_row) {
		gtk_list_box_select_row(GTK_LIST_BOX(sidebar_list), first_row);
	}

	/* show */
	gtk_window_present(GTK_WINDOW(state->window));
}

int
main(int argc, char **argv)
{
#if HAVE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
#endif
	struct state state = { 0 };

	/* read/create config file */
	char filename[4096];
	char *home = getenv("HOME");
	snprintf(filename, sizeof(filename), "%s/%s", home, ".config/labwc/rc.xml");
	xml_init(filename);
	xml_setup_nodes();

	/* connect to gsettings */
	state.settings = g_settings_new("org.gnome.desktop.interface");

	/* start ui */
	AdwApplication *app;
	int status;
	app = adw_application_new("org.labwc.tweaks", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), &state);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	/* clean up */
	xml_finish();

	return status;
}
