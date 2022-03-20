/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+
#+     Glade / Gtk Programming
#+
#+     Copyright (C) 2019 by Kevin C. O'Kane
#+
#+     Kevin C. O'Kane
#+     kc.okane@gmail.com
#+     https://www.cs.uni.edu/~okane
#+     http://threadsafebooks.com/
#+
#+ This program is free software; you can redistribute it and/or modify
#+ it under the terms of the GNU General Public License as published by
#+ the Free Software Foundation; either version 2 of the License, or
#+ (at your option) any later version.
#+
#+ This program is distributed in the hope that it will be useful,
#+ but WITHOUT ANY WARRANTY; without even the implied warranty of
#+ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#+ GNU General Public License for more details.
#+
#+ You should have received a copy of the GNU General Public License
#+ along with this program; if not, write to the Free Software
#+ Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#+
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// Jan 8, 2022

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <sys/ipc.h>
#include <math.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fftw3.h>
#include <gtk/gtkx.h>
#include <time.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include "functions.h"

#include "global-variables.h"

int main(int argc, char *argv[]) {

	signal(SIGPIPE, SIG_IGN);

	context_source = NULL;
	context_sink = NULL;
	stream_sink = NULL;
	stream_source = NULL;
	device_name_source = NULL;
	device_name_sink = NULL;
	device_description_source = NULL;
	device_description_sink = NULL;
	int record = 0;
	showArc = 0;
	showB2 = 1;
	connect_graph = TRUE; // whether to connect the bar graph bars
	spectrum_width = 1;
	display_cover = 0;
	no_graph = 0;
	legacy_vumeters = 0; 
	mono_graph = 0;
	timer_res = 50;
	no_meters = 0;
	LAT = 3;
	rand_colors = 0;
	strcpy(bg_color,"black");
	L_Clipping = 0;
	R_Clipping = 0;
	style = NARROW_SPECTRUM; // narrow
	FACTOR = 350.0;
	Lx = 0.1;
	Rx = 0.1;
	strcpy(default_source,"");
	strcpy(default_sink,"");
	no_microphone = 1;
	exchange = 0;
	super_bars = 1;         // bars above graphs
	MaxR[0] = 0.0;
	MaxR[1] = 0.0;
	MaxL[0] = 0.0;
	MaxL[1] = 0.0;
	MaxM[0] = 0.0;
	MaxM[1] = 0.0;
	MaxRM = 0.0;
	MaxLM = 0.0;
	MaxMM = 0.0;
	MRr = 0;
	MLr = 0;
	MMr = 0;
	ML = 0.0;
	MR = 0.0;
	Balance = 0.0;
	MLx = 0.0;
	MRx = 0.0;

	gtk_init(&argc, &argv); // init Gtk

	printf("\n-----------------------------------------------\n");

//-----------------------
//	GTK interface
//-----------------------

	builder = gtk_builder_new_from_resource ("/part1/part1.glade");
 
	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

	g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);

        gtk_builder_connect_signals(builder, NULL);

	fixed1		= GTK_WIDGET(gtk_builder_get_object(builder, "fixed1"));
	Bars		= GTK_WIDGET(gtk_builder_get_object(builder, "Bars"));
	draw1		= GTK_WIDGET(gtk_builder_get_object(builder, "draw1"));
	draw2		= GTK_WIDGET(gtk_builder_get_object(builder, "draw2"));
	Spectrum	= GTK_WIDGET(gtk_builder_get_object(builder, "Spectrum"));
	meters		= GTK_WIDGET(gtk_builder_get_object(builder, "meters"));
	balance		= GTK_WIDGET(gtk_builder_get_object(builder, "balance"));
	vumeter1	= GTK_WIDGET(gtk_builder_get_object(builder, "vumeter1"));
	vumeter2	= GTK_WIDGET(gtk_builder_get_object(builder, "vumeter2"));
	vumeter3	= GTK_WIDGET(gtk_builder_get_object(builder, "vumeter3"));

	g_object_unref(builder);

//	gtk_window_set_keep_above (GTK_WINDOW(window), TRUE); // un-comment to keep on top

	gtk_widget_show(window);

        GdkColor color; // default background color

	if (!gdk_color_parse (bg_color, &color)) { // does color parse?
        	color.blue = 0x0000;
        	color.red = 0x0000;
        	color.green = 0x0000;
		}

        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &color); // set background color

	char cmd[1024];
	strcpy(cmd,"Spark Gap Radio Meters 2.1");
	gtk_window_set_title(GTK_WINDOW(window),cmd);

	style = NARROW_SPECTRUM; 

//----------------------------------------------------------------------
//	Process command line arguments
//----------------------------------------------------------------------

//	-a	show arcs
//	-b	show bars only
//	-c	centered spectrum
//	-g	spectrum graph only - no meters
//	-l	lines only
//	-m	meters only - no spectrum graph
//	-B	bars and lines
//	-w	wide spectrum
//	-v	vu meters
//	-s val	sample rate [50]
//	-h	help
//	-?	help

	int flags, opt;

           while ((opt = getopt(argc, argv, "abcglmBwh?vs:")) != -1) {

               switch (opt) {

               case 'a':
		showArc = 1;
                   break;

               case 'b':
		connect_graph = FALSE;
                   break;

               case 'c':
		super_bars = 0;
                   break;

               case 'g':
		no_meters = 1;
		gtk_widget_hide(draw2);
		gtk_window_resize(window, 20, 20);
		strcpy(cmd,"Spark Gap Radio Engineering Dept Spectrum 2.00");
		gtk_window_set_title(GTK_WINDOW(window),cmd);
                   break;

               case 'l':
		connect_graph = TRUE;
                   break;

               case 'm':
		no_graph = TRUE;
		gtk_fixed_move (
  				fixed1,
  				draw2,
  				1, 20);
		gtk_window_resize(window, 20, 20);
		strcpy(cmd,"Spark Gap Radio Engineering Dept VU Meters 2.00");
		gtk_window_set_title(GTK_WINDOW(window),cmd);
                   break;

               case 'B':
		connect_graph = TRUE;
		display_bars = 1;
                   break;

               case 'w':
		style = WIDE_SPECTRUM; 
                   break;

               case 'v':
		no_graph = TRUE;
		legacy_vumeters = 1;
		gtk_widget_hide(draw1);
		gtk_fixed_move (
  				fixed1,
  				draw2,
  				1, 20);
		gtk_widget_set_size_request (draw2, 580, 74);
		gtk_window_resize(window, 20, 20);
		strcpy(cmd,"Spark Gap Radio Engineering Dept VU Meters 2.00");
		gtk_window_set_title(GTK_WINDOW(window),cmd);
                   break;

               case 's':
                   timer_res = atoi(optarg);
                   break;

               case 'h':
               case '?':
               default:
                   fprintf(stderr, "Usage: -ablBwh?s\n");
			fprintf(stderr,"-a	show arcs\n");
			fprintf(stderr,"-b	show bars only\n");
			fprintf(stderr,"-c	centered spectrum\n");
			fprintf(stderr,"-g	spectrum graph only - no meters\n");
			fprintf(stderr,"-l	lines only\n");
			fprintf(stderr,"-m	meters only - no spectrum graph\n");
			fprintf(stderr,"-B	bars and lines\n");
			fprintf(stderr,"-w	wide spectrum\n");
			fprintf(stderr,"-v	vu meters\n");
			fprintf(stderr,"-s val	sample rate [50]\n");
			fprintf(stderr,"-h	help\n");
			fprintf(stderr,"-?	help\n");
                   exit(EXIT_FAILURE);
               }

           }

	if (! legacy_vumeters) {
		gtk_widget_hide(vumeter1);
		gtk_widget_hide(vumeter2);
		gtk_widget_hide(vumeter3);
		gtk_window_resize(window, 20, 20);
		}

#include "sink.h"

        g_timeout_add(timer_res, (GSourceFunc) pulse_timer_handler, NULL);
            
	gtk_main();

	return EXIT_SUCCESS;
	}

