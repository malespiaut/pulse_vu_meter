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

// August 14, 2021

extern int	display_bars;

extern pa_glib_mainloop        *m_sink;        // internal audio - sink
extern pa_glib_mainloop        *m_source;      // pulse audio microphone - source

extern pa_context       *context_source;
extern pa_context       *context_sink;
extern pa_stream        *stream_sink;
extern pa_stream        *stream_source;
extern char             *device_name_source;
extern char             *device_name_sink;
extern char             *device_description_source;
extern char             *device_description_sink;

extern GtkWidget       *window;
extern GtkWidget       *fixed1;
extern GtkWidget       *draw1;
extern GtkWidget       *draw2;
extern GtkWidget       *Bars;
extern GtkWidget       *Spectrum;
extern GtkWidget       *meters;
extern GtkWidget       *balance;
extern GtkBuilder      *builder;

extern int samp1,samp2;
extern int showArc;
extern int showB2;
extern int connect_graph; // whether to connect the bar graph bars
extern int spectrum_width;
extern int display_cover;
extern int no_graph;
extern int legacy_vumeters;
extern int mono_graph;
extern int timer_res;
extern int no_meters;

extern int LAT;
extern int rand_colors;
extern char bg_color[256];
extern int  L_Clipping, R_Clipping;
extern int  style;

extern void                    bar_graph(int lower, double[], int, cairo_t *, int, int);
extern void                    color_bars(cairo_t *cr, int, int, int);
extern void                    frequency_marks(cairo_t *);
extern void                    draw_bars_only(cairo_t *cr, int lower, int i, double chan[]);
extern void                    connecting_lines(cairo_t *, int , int , double []);
extern gboolean                on_draw1_draw (GtkDrawingArea *, cairo_t *) ;
extern gboolean                on_draw2_draw (GtkDrawingArea *widget, cairo_t *cr);
extern int                     style; // narrow
extern double                  FACTOR, LeftChan, RightChan, Lx, Rx;
extern double                  rUtil, srchan[100], rchan[100];
extern double                  lUtil, slchan[100], lchan[100];
extern char                    default_source[1024];
extern char                    default_sink[1024];
extern int                     no_microphone;
extern float                   exchangeBuf[SAMPLE_SIZE];
extern int                     exchange;
extern int                     sink_channels, source_channels;
extern int                     super_bars;         // bars above graphs

#define RMX 5
extern double                  MaxR[RMX], MaxL[RMX], MaxM[RMX];
extern double                  MaxRM, MaxLM, MaxMM;
extern int                     MRr, MLr, MMr;

extern double                  ML, MR, Balance;
extern double                  MLx, MRx;

