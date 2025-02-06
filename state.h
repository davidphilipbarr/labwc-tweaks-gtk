/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef STATE_H
#define STATE_H
#include <gtk/gtk.h>
#include "config.h"
#if HAVE_NLS
#include <libintl.h>
#include <locale.h>
#define _ gettext
#else
#define _(s) (s)
#endif

struct state {
	GtkWidget *window;
	struct {
		GtkWidget *corner_radius;
		GtkWidget *openbox_theme_name;
		GtkWidget *gtk_theme_name;
		GtkWidget *icon_theme_name;
		GtkWidget *cursor_theme_name;
		GtkWidget *cursor_size;
		GtkWidget *natural_scroll;
		GtkWidget *keyboard_layout;
		GtkWidget *drop_shadows;
		GtkWidget *button_layout;
		GtkWidget *show_title;
		GtkWidget *double_click_time;
		GtkWidget *top_max;
		GtkWidget *placement;
		GtkWidget *xwayland_persistence;
		GtkWidget *allow_tearing;
		GtkWidget *adaptive_sync;
		GtkWidget *follow_mouse;
		GtkWidget *follow_mouse_requires_movement;
		GtkWidget *raise_on_focus;
		GtkWidget *prefer_dark;
		GtkWidget *gap;
	        GtkWidget *corner_range;		
		GtkWidget *draw_contents;		
		GtkWidget *popup_show;
		GtkWidget *label;
		    GtkWidget *file_button;
			    GtkWidget *icon_path;
    GtkWidget *icon_preview;
    GtkWidget *font_button;
    GtkWidget *active_font_button;
    GtkWidget *inactive_font_button;

	} widgets;
	GSettings *settings;
};

#endif /* STATE_H */
