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

int	display_bars;

pa_glib_mainloop        *m_sink;        // internal audio - sink
pa_glib_mainloop        *m_source;      // pulse audio microphone - source

pa_context       *context_source;
pa_context       *context_sink;
pa_stream        *stream_sink;
pa_stream        *stream_source;
char             *device_name_source;
char             *device_name_sink;
char             *device_description_source;
char             *device_description_sink;

GtkWidget       *window;
GtkWidget       *fixed1;
GtkWidget       *draw1;
GtkWidget       *draw2;
GtkWidget       *Bars;
GtkWidget       *Spectrum;
GtkWidget       *meters;
GtkWidget       *balance;
GtkWidget       *vumeter1;
GtkWidget       *vumeter2;
GtkWidget       *vumeter3;
GtkBuilder      *builder;

int samp1,samp2;
int showArc;
int showB2;
int connect_graph; // whether to connect the bar graph bars
int spectrum_width;
int display_cover;
int no_graph;
int legacy_vumeters;
int mono_graph;
int timer_res;
int no_meters;

int LAT;
int rand_colors;
char bg_color[256];
int  L_Clipping, R_Clipping;
int  style;

void                    bar_graph(int lower, double[], int, cairo_t *, int, int);
void                    color_bars(cairo_t *cr, int, int, int);
void                    frequency_marks(cairo_t *);
void                    draw_bars_only(cairo_t *cr, int lower, int i, double chan[]);
void                    connecting_lines(cairo_t *, int , int , double []);
gboolean                on_draw1_draw (GtkDrawingArea *, cairo_t *) ;
gboolean                on_draw2_draw (GtkDrawingArea *widget, cairo_t *cr);
int                     style; // narrow
double                  FACTOR, LeftChan, RightChan, Lx, Rx;
double                  rUtil, srchan[100], rchan[100];
double                  lUtil, slchan[100], lchan[100];
char                    default_source[1024];
char                    default_sink[1024];
int                     no_microphone;
float                   exchangeBuf[SAMPLE_SIZE];
int                     exchange;
int                     sink_channels, source_channels;
int                     super_bars;         // bars above graphs

double                  MaxR[RMX], MaxL[RMX], MaxM[RMX];
double                  MaxRM, MaxLM, MaxMM;
int                     MRr, MLr, MMr;

double                  ML, MR, Balance;
double                  MLx, MRx;

