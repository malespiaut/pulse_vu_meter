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

// August 20, 2021

pa_glib_mainloop* m_sink = 0;   // internal audio - sink
pa_glib_mainloop* m_source = 0; // pulse audio microphone - source

pa_context* context_source = 0;
pa_context* context_sink = 0;
pa_stream* stream_sink = 0;
pa_stream* stream_source = 0;
char* device_name_source = 0;
char* device_name_sink = 0;
char* device_description_source = 0;
char* device_description_sink = 0;

GtkWidget* window = 0;
GtkWidget* fixed1 = 0;
GtkWidget* draw1 = 0;
GtkWidget* draw2 = 0;
GtkWidget* Bars = 0;
GtkWidget* Spectrum = 0;
GtkWidget* meters = 0;
GtkWidget* balance = 0;
GtkWidget* vumeter1 = 0;
GtkWidget* vumeter2 = 0;
GtkWidget* vumeter3 = 0;
GtkBuilder* builder = 0;

int samp1, samp2;
int showArc = 0;
int showB2 = 1;
int connect_graph = TRUE; // whether to connect the bar graph bars
int spectrum_width = 1;
int display_cover = 0;
int no_graph = 0;
int legacy_vumeters = 0;
int mono_graph = 0;
int timer_res = 50;
int no_meters = 0;

int LAT = 3;
int rand_colors = 0;
char bg_color[256] = "black";
int L_Clipping = 0;
int R_Clipping = 0;
int style = NARROW_SPECTRUM;

void bar_graph(int lower, double[], int, cairo_t*, int);
void color_bars(cairo_t* cr, int, int, int);
void frequency_marks(cairo_t*);
void draw_bars_only(cairo_t* cr, int lower, int i, double chan[]);
void connecting_lines(cairo_t*, int, int, double[]);
gboolean on_draw1_draw(GtkDrawingArea*, cairo_t*);
gboolean on_draw2_draw(GtkDrawingArea* widget, cairo_t* cr);
double FACTOR = 350.0;
double LeftChan = NAN;
double RightChan = NAN;
double Lx = 0.1;
double Rx = 0.1;
double rUtil = NAN;
double srchan[100] = {NAN};
double rchan[100] = {NAN};
double lUtil = NAN;
double slchan[100] = {NAN};
double lchan[100] = {NAN};
char default_source[1024] = {0};
char default_sink[1024] = {0};
int no_microphone = 1;
float exchangeBuf[SAMPLE_SIZE] = {NAN};
int exchange = 0;
int sink_channels = 0;
int source_channels = 0;
int super_bars = 1; // bars above graphs

double MaxR[RMX] = {0.0};
double MaxL[RMX] = {0.0};
double MaxM[RMX] = {0.0};
double MaxRM = 0.0;
double MaxLM = 0.0;
double MaxMM = 0.0;
int MRr = 0;
int MLr = 0;
int MMr = 0;

double ML = 0.0;
double MR = 0.0;
double Balance = 0.0;
double MLx = 0.0;
double MRx = 0.0;
