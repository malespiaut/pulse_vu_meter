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

// August 12, 2021

//----------------------------------------------
//	PulseAudio interface
//	SINK (players) SINK SINK SINK SINK SINK
//----------------------------------------------

// PLAYER SINK

printf("\n------------------ sink connect --------\n");

GMainContext* mcd_sink;

mcd_sink = g_main_context_default();

m_sink = pa_glib_mainloop_new(mcd_sink);

pa_mainloop_api* mla_sink;

mla_sink = pa_glib_mainloop_get_api(m_sink);

context_sink = pa_context_new(mla_sink, PULSE_SINK_NAME);

pa_context_set_state_callback(
  context_sink,
  sink_connection_state_callback,
  0);

if (pa_context_connect(context_sink, 0, PA_CONTEXT_NOAUTOSPAWN, 0) < 0)
  printf("*** sink context connect error\n");

printf("*** sink connected\n");

// MICROPHONE SOURCE

printf("\n------------------ source connect --------\n");

GMainContext* mcd_source;

mcd_source = g_main_context_default();

m_source = pa_glib_mainloop_new(mcd_source);

pa_mainloop_api* mla_source;

mla_source = pa_glib_mainloop_get_api(m_source);

context_source = pa_context_new(mla_source, PULSE_SOURCE_NAME);

pa_context_set_state_callback(
  context_source,
  source_connection_state_callback,
  0);

if (pa_context_connect(context_source, 0, PA_CONTEXT_NOAUTOSPAWN, 0) < 0)
  printf("*** source context connect error\n");

printf("*** source connected\n");
