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

// August 15, 2021

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <pulse/error.h>

#include <gtk/gtk.h>
#include <sys/ipc.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fftw3.h>
#include <gtk/gtkx.h>

void	show_error_sink(const char *txt );

void	show_error_source(const char *txt );

void	stream_read_callback_sink(pa_stream *s, size_t l, void *dmy);

void	stream_read_callback_source(pa_stream *s, size_t len, void *user);

void	stream_state_callback_source(pa_stream *s, void *dmy);

void	sink_create_stream(const char *name, const char *description, 
		const pa_sample_spec sampSpec, const pa_channel_map cmap);

void	source_create_stream(const char *name, const char *description, 
		const pa_sample_spec sampSpec, const pa_channel_map cmap);

void	source_info_callback(pa_context *p, const pa_source_info *si, int is_last, void *dmy);

void	sink_info_callback(pa_context *p, const pa_sink_info *si, int is_last, void *dmy);

void	sink_server_info_callback(pa_context *c, const pa_server_info *si, void *dmy);

void	source_server_info_callback(pa_context *c, const pa_server_info *si, void *dmy);

void    source_connection_state_callback(pa_context *c, void *dmy);

void	sink_connection_state_callback(pa_context *c, void *dmy);

void	on_destroy();

void spectrum();

gboolean pulse_timer_handler();

#define SAMPLE_SIZE 400
#define PULSE_TIME 100 // granualrity of pulse calls - see sink.h

#define NEEDLES 1
#define ARC 2
#define FILLED_ARC 3
#define BARGRAPH 1
#define BARS 2
#define GRAPHS 3
#define METERS 4
#define WIDE_SPECTRUM 5
#define NARROW_SPECTRUM 6

#define CLIP_LEVEL 103.0

char *	sink_name();
char *	source_name();

#define PULSE_SINK_NAME sink_name()
#define PULSE_SOURCE_NAME source_name()

#define RMX 5

