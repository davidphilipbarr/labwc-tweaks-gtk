/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef UPDATE_H
#define UPDATE_H
#include <gtk/gtk.h>

void update(GtkWidget *widget, gpointer data);
void on_update_clicked(GtkButton *btn, gpointer user_data);
void on_quit_clicked(GtkButton *btn, gpointer user_data);

#endif /* UPDATE_H */
