// SPDX-License-Identifier: GPL-2.0-only
#include "keyboard-layouts.h"
#include "state.h"
#include "stack-behaviour.h"
#include "theme.h"
#include "xml.h"

void
stack_behaviour_init(struct state *state, GtkWidget *stack)
{
	GtkWidget *widget;

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_stack_add_named(GTK_STACK(stack), vbox, "behaviour");
	gtk_container_child_set(GTK_CONTAINER(stack), vbox, "title", "Behaviour", NULL);

	/* the grid with settings */
	int row = 0;
	GtkWidget *grid = gtk_grid_new();
	g_object_set(grid, "margin", 20, "row-spacing", 10, "column-spacing", 10, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 5);

		/*  */
	widget = gtk_label_new(_("Top Maximize"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.top_max = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.top_max), "no");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.top_max), "yes");
      
	gtk_grid_attach(GTK_GRID(grid), state->widgets.label, 1, row++, 1, 1);
		/*gaps */
	widget = gtk_label_new(_("Gap"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	GtkAdjustment *adjustment = gtk_adjustment_new(0, 0, 20, 1, 2, 0);
	state->widgets.gap = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->widgets.gap), xml_get_int("/labwc_config/core/gap"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.gap, 1, row++, 1, 1);
	
	/* Window Placement */
	widget = gtk_label_new(_("Window Placement"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.placement = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.placement), "Automatic");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.placement), "Center");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.placement), "Cascade");
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.placement), xml_get_int("/labwc_config/placement/policy"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.placement, 1, row++, 1, 1);
	
	

  	/*   <adaptiveSync>no</adaptiveSync> */
		
	widget = gtk_label_new(_("Adaptive Sync"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.adaptive_sync = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.adaptive_sync), "no");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.adaptive_sync), "yes");
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.adaptive_sync), xml_get_bool_text("/labwc_config/core/adaptiveSync"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.adaptive_sync, 1, row++, 1, 1);
  
  
   /* <allowTearing>no</allowTearing> */
		
	widget = gtk_label_new(_("Allow Tearing"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.allow_tearing = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.allow_tearing ), "no");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.allow_tearing ), "yes");
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.allow_tearing), xml_get_bool_text("/labwc_config/core/allowTearing"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.allow_tearing, 1, row++, 1, 1);
  
  
   /*  <xwaylandPersistence>no</xwaylandPersistence> */
	
	widget = gtk_label_new(_("Xwayland Persistence"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.xwayland_persistence = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.xwayland_persistence  ), "no");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.xwayland_persistence ), "yes");
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.xwayland_persistence ), xml_get_bool_text("/labwc_config/core/xwaylandPersistence"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.xwayland_persistence , 1, row++, 1, 1);


   /*
  <resize>
    <!-- Show a simple resize and move indicator -->
    <popupShow>Never</popupShow>
    <!-- Let client redraw its contents while resizing -->
    <drawContents>yes</drawContents>
    <cornerRange>8</cornerRange>
  </resize>
 */
 
 	widget = gtk_label_new(_("Show Resize Popup"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.popup_show = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.popup_show), "Nonpixel");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.popup_show), "Always");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.popup_show), "Never");
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.popup_show), xml_get_int("/labwc_config/resize/popupShow"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.popup_show, 1, row++, 1, 1);
 
        widget = gtk_label_new(_("Draw Window Contents"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	state->widgets.draw_contents = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.draw_contents), "no");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(state->widgets.draw_contents), "yes");
	gtk_combo_box_set_active(GTK_COMBO_BOX(state->widgets.draw_contents), xml_get_bool_text("/labwc_config/resize/drawContents"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.draw_contents, 1, row++, 1, 1);
 
 	/* corner radius spinbutton */
	widget = gtk_label_new(_("Corner Range"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	GtkAdjustment *adjustmentr = gtk_adjustment_new(0, 0, 20, 1, 2, 0);
	state->widgets.corner_range = gtk_spin_button_new(GTK_ADJUSTMENT(adjustmentr), 1, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->widgets.corner_range), xml_get_int("/labwc_config/resize/cornerRange"));
	gtk_grid_attach(GTK_GRID(grid), state->widgets.corner_range, 1, row++, 1, 1);
 
}

