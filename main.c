

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fftw3.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <math.h>
#include <pulse/error.h>
#include <pulse/glib-mainloop.h>
#include <pulse/pulseaudio.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void on_destroy(void);
void show_error_sink(const char*);
void show_error_source(const char*);
void sink_connection_state_callback(pa_context*, void*);
void sink_create_stream(const char*, const char*, const pa_sample_spec, const pa_channel_map);
void sink_info_callback(pa_context*, const pa_sink_info*, int, void*);
void sink_server_info_callback(pa_context*, const pa_server_info*, void*);
void source_connection_state_callback(pa_context*, void*);
void source_create_stream(const char*, const char*, const pa_sample_spec, const pa_channel_map);
void source_info_callback(pa_context*, const pa_source_info*, int, void*);
void source_server_info_callback(pa_context*, const pa_server_info*, void*);
void spectrum(void);
void stream_read_callback_sink(pa_stream*, size_t, void*);
void stream_read_callback_source(pa_stream*, size_t, void*);
void stream_state_callback_source(pa_stream*, void*);

gboolean pulse_timer_handler(gpointer);

#define SAMPLE_SIZE 400
#define PULSE_TIME 100

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

char* sink_name(void);
char* source_name(void);

#define PULSE_SINK_NAME sink_name()
#define PULSE_SOURCE_NAME source_name()

#define RMX 5

#define RMX 5

#define DRAW_HEIGHT 105.0

float UPPER_MAX = DRAW_HEIGHT - DRAW_HEIGHT / 10.0;
float UPPER_MAX_MAX = DRAW_HEIGHT - DRAW_HEIGHT / 20.0;

#define SPECTRUM_PEAK DRAW_HEIGHT - 30.0

#define SCALE_BASELINE_SPECTRUM DRAW_HEIGHT / 160.0
#define SCALE_MIDLINE_SPECTRUM DRAW_HEIGHT / 250.0

#define SCALE_BASELINE_VOLUME DRAW_HEIGHT / 150.0

#define UPPER_MAX_MAX_COLOR cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
#define UPPER_MAX_COLOR cairo_set_source_rgb(cr, 1.0, 0.7, 0.7);

#define RED 1
#define YELLOW 2
#define WHITE 3
#define G_OFF 4
#define G_OFF1 4
#define UPPER 0
#define LOWER 1

#define BAR_WIDTH1 2.9

#define G_MAX DRAW_HEIGHT / 2.0 - 10.0
#define G_MAX2 DRAW_HEIGHT - 20
#define G_LIM DRAW_HEIGHT

#define MHOR 55.0
#define DECAY_FACTOR 250.0

#define HOR_ORIGIN 32.0
#define VER_ORIGIN 78.0
#define BAR_WIDTH 3.25
#define LINE_WIDTH 5
#define LINE_SEP 4.0

#define WindowWidth 200

#define SPECTRUM_FREQ_VERTICAL DRAW_HEIGHT - 10

int maxHeight = 100;

pa_glib_mainloop* m_sink = 0;
pa_glib_mainloop* m_source = 0;

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

int samp1 = 0;
int samp2 = 0;
int showArc = 0;
int showB2 = 1;
gboolean connect_graph = TRUE;
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

void bar_graph(int, double[], size_t, cairo_t*, gboolean);
void color_bars(cairo_t*, size_t, int, int);
void frequency_marks(cairo_t*);
void draw_bars_only(cairo_t*, int, size_t, double[]);
void connecting_lines(cairo_t*, int, size_t, double[]);
gboolean on_draw1_draw(GtkDrawingArea*, cairo_t*);
gboolean on_draw2_draw(GtkDrawingArea*, cairo_t*);
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
size_t sink_channels = 0;
size_t source_channels = 0;
int super_bars = 1;

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

gboolean
pulse_timer_handler(gpointer user_data)
{

  (void)user_data;

  rUtil = log10(RightChan + 1) * FACTOR;
  lUtil = log10(LeftChan + 1) * FACTOR;

  LeftChan = RightChan = 0.0;

  for (size_t i = 99; i > 0; i--)
    {
      rchan[i] = rchan[i - 1];
    }
  for (size_t i = 99; i > 0; i--)
    {
      lchan[i] = lchan[i - 1];
    }

  rchan[0] = rUtil;
  lchan[0] = lUtil;

  gtk_widget_queue_draw(draw1);
  gtk_widget_queue_draw(draw2);
  return TRUE;
}

void
needles(cairo_t* cr, double hor, double ver, double len, double t1, double t2, int cFlg, double Max)
{

  double x, x1, y, y1;
  double X = 0.0;

  ver = ver + 1.0;

  if (t1 > M_PI)
    {
      t1 = M_PI;
    }

  X = 0.0;

  if (!legacy_vumeters)
    {

      if (cFlg == 0)
        {
          cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        }
      else if (cFlg == 1)
        {
          cairo_set_source_rgb(cr, 1.0, 1.0, 0.3);
        }
      else if (cFlg == 2)
        {
          cairo_set_source_rgb(cr, 1.0, 0., 0.);
        }
      else if (cFlg == 3)
        {
          cairo_set_source_rgb(cr, 0.5, 1.0, 0.5);
        }
      else if (cFlg == 4)
        {
          cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        }

      cairo_set_line_width(cr, 2.5);

      for (size_t i = 0; i < 21; i++)
        {

          if (!(i % 2))
            {

              cairo_arc(cr, hor, ver, len + 10, -M_PI, -M_PI + X);
              cairo_get_current_point(cr, &x, &y);

              cairo_arc(cr, hor, ver, len, -M_PI, -M_PI + X);
              cairo_get_current_point(cr, &x1, &y1);

              cairo_new_path(cr);
              cairo_move_to(cr, x, y);
              cairo_line_to(cr, x1, y1);
              cairo_stroke(cr);
            }

          else
            {
              cairo_arc(cr, hor, ver, len + 2, -M_PI, -M_PI + X);
              cairo_get_current_point(cr, &x, &y);

              cairo_arc(cr, hor, ver, len - 1, -M_PI, -M_PI + X);
              cairo_get_current_point(cr, &x1, &y1);
            }

          if (X > t1)
            {

              cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
              cairo_new_path(cr);
              cairo_move_to(cr, x, y);
              cairo_line_to(cr, x1, y1);
              cairo_stroke(cr);

              if (cFlg != 4)
                {
                  X = X + M_PI / 20.0;
                  continue;
                }
            }

          cairo_new_path(cr);
          cairo_move_to(cr, x, y);
          cairo_line_to(cr, x1, y1);
          cairo_stroke(cr);

          X = X + M_PI / 20.0;
        }

      if (Max > 0.0)
        {

          cairo_set_line_width(cr, 5.0);

          if (Max > M_PI)
            {
              Max = M_PI;
            }

          if (cFlg == 1)
            {
              cairo_set_source_rgb(cr, 1.0, 1.0, 0.3);
            }
          else if (cFlg == 2)
            {
              cairo_set_source_rgb(cr, 1.0, 0., 0.);
            }
          else if (cFlg == 3)
            {
              cairo_set_source_rgb(cr, 0.5, 1.0, 0.5);
            }
          else if (cFlg == 4)
            {
              cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
            }

          cairo_arc(cr, hor, ver, len + 10, -M_PI, -M_PI + Max);
          cairo_get_current_point(cr, &x, &y);

          cairo_arc(cr, hor, ver, len + 16, -M_PI, -M_PI + Max);
          cairo_get_current_point(cr, &x1, &y1);

          cairo_new_path(cr);
          cairo_move_to(cr, x, y);
          cairo_line_to(cr, x1, y1);
          cairo_stroke(cr);
        }

      if (t2 > 0.0)
        {
          cairo_set_line_width(cr, 2.5);

          if (t2 > M_PI)
            {
              t2 = M_PI;
            }

          if (cFlg == 1)
            {
              cairo_set_source_rgb(cr, 1.0, 1.0, 0.3);
            }
          else if (cFlg == 2)
            {
              cairo_set_source_rgb(cr, 1.0, 0., 0.);
            }
          else if (cFlg == 3)
            {
              cairo_set_source_rgb(cr, 0.5, 1.0, 0.5);
            }
          else if (cFlg == 4)
            {
              cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
            }

          cairo_arc(cr, hor, ver, len + 10, -M_PI, -M_PI + t2);
          cairo_get_current_point(cr, &x, &y);

          cairo_arc(cr, hor, ver, len + 16, -M_PI, -M_PI + t2);
          cairo_get_current_point(cr, &x1, &y1);

          cairo_new_path(cr);
          cairo_move_to(cr, x, y);
          cairo_line_to(cr, x1, y1);
          cairo_stroke(cr);
        }
    }

  cairo_set_line_width(cr, 2.5);

  if (cFlg == 0)
    {
      cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    }
  else if (cFlg == 1)
    {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.3);
    }
  else if (cFlg == 2)
    {
      cairo_set_source_rgb(cr, 1.0, 0., 0.);
    }
  else if (cFlg == 3)
    {
      cairo_set_source_rgb(cr, 0.5, 1.0, 0.5);
    }
  else if (cFlg == 4)
    {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    }

  cairo_set_line_width(cr, 3.5);

  cairo_arc(cr, hor, ver, len, -M_PI, -M_PI + t1);

  cairo_get_current_point(cr, &x, &y);

  cairo_new_path(cr);
  cairo_move_to(cr, x, y);
  cairo_line_to(cr, hor, ver);
  cairo_stroke(cr);

  if (legacy_vumeters)
    {
      return;
    }

  if (cFlg == 4)
    {

      if (t1 < M_PI_2)
        {

          cairo_set_source_rgb(cr, 1.0, 1.0, 0.3);

          cairo_arc(cr, hor, ver, len - 2, -M_PI + t1, -M_PI_2);
          cairo_line_to(cr, hor, ver);
          cairo_fill(cr);
          cairo_stroke(cr);
        }

      else
        {
          cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

          cairo_arc(cr, hor, ver, len - 2, -M_PI_2, -M_PI + t1);
          cairo_line_to(cr, hor, ver);
          cairo_fill(cr);
          cairo_stroke(cr);
        }

      cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    }

  if (cFlg != 4 && showArc)
    {

      cairo_arc(cr, hor, ver, len - 2, -M_PI, -M_PI + t1);
      cairo_line_to(cr, hor, ver);
      cairo_fill(cr);
      cairo_stroke(cr);
    }

  cairo_arc(cr, hor, ver, 4.0, -M_PI, M_PI);
  cairo_line_to(cr, hor, ver);
  cairo_fill(cr);
}

gboolean
on_draw1_draw(GtkDrawingArea* widget, cairo_t* cr)
{
  (void)widget;

  int noLine = 0;

  if (no_graph)
    {
      return FALSE;
    }

  if (style == NARROW_SPECTRUM || style == WIDE_SPECTRUM)
    {

      double blchan[100] = {NAN};
      double brchan[100] = {NAN};

      for (size_t i = 0; i < 100; i++)
        {
          blchan[i] = slchan[i];
          brchan[i] = srchan[i];
        }

      spectrum();
      noLine = 1;

      for (size_t i = 0; i < 99; i++)
        {

          slchan[i] = (slchan[i] + blchan[i]) / 2.0;
          srchan[i] = (srchan[i] + brchan[i]) / 2.0;
        }
    }

  else
    {
      if (super_bars)
        {
          for (size_t i = 0; i < 100; i++)
            {
              slchan[i] = lchan[i] * SCALE_BASELINE_VOLUME;
              srchan[i] = rchan[i] * SCALE_BASELINE_VOLUME;
            }
        }
      else
        {
          for (size_t i = 0; i < 100; i++)
            {
              slchan[i] = lchan[i];
              srchan[i] = rchan[i];
            }
        }
    }

  cairo_set_line_width(cr, BAR_WIDTH1);

  for (size_t i = 0; i < 99; i++)
    {

      if (!connect_graph && super_bars)
        {

          if (slchan[i] > srchan[i])
            {
              color_bars(cr, i, noLine, YELLOW);
              bar_graph(UPPER, slchan, i, cr, connect_graph);
            }
          else
            {
              color_bars(cr, i, noLine, RED);
              bar_graph(LOWER, srchan, i, cr, connect_graph);
            }
        }

      else
        {

          color_bars(cr, i, noLine, YELLOW);
          bar_graph(UPPER, slchan, i, cr, connect_graph);

          color_bars(cr, i, noLine, RED);
          bar_graph(LOWER, srchan, i, cr, connect_graph);
        }
    }

  if (!super_bars)
    {
      cairo_set_line_width(cr, 1.0);
      cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
      cairo_move_to(cr, (double)G_OFF - 2, DRAW_HEIGHT / 2.0 - 10.0);
      cairo_line_to(cr, (double)G_OFF + 394, DRAW_HEIGHT / 2.0 - 10.0);
      cairo_stroke(cr);
    }

  frequency_marks(cr);

  return TRUE;
}

void
color_bars(cairo_t* cr, size_t i, int noLine, int color)
{

  if ((style == WIDE_SPECTRUM || style == NARROW_SPECTRUM) && !super_bars)
    {
      rand_colors = 1;
    }
  else
    {
      rand_colors = 0;
    }

  if (!rand_colors)
    {
      if (!noLine && srchan[i] > UPPER_MAX_MAX)
        {
          UPPER_MAX_MAX_COLOR
        }
      else if (!noLine && srchan[i] > UPPER_MAX)
        {
          UPPER_MAX_COLOR
        }
      else if (color == RED)
        {
          cairo_set_source_rgb(cr, 1.0, 0.1, 0.1);
        }
      else if (color == YELLOW)
        {
          cairo_set_source_rgb(cr, 1.0, 1.0, 0.3);
        }
      else if (color == WHITE)
        {
          cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        }
      else
        {
          cairo_set_source_rgb(cr, 0.2, 0.2, 1.0);
        }
    }

  else
    {
      float r = (100 - i) / 100.0;
      float b = (i) / 100.0 + 0.1;
      float g;

      if (i == 0)
        {
          g = 0.0;
        }
      else if (i == 50)
        {
          g = 1.0;
        }
      else if (i < 50)
        {
          g = (i % 50) / 50.0;
        }
      else
        {
          g = ((100 - i) % 50) / 50.0;
        }

      g = g + r / (30.0 / i);
      r = r + b / (i / 40.0);
      cairo_set_source_rgb(cr, r, g, b);
    }
}

void
bar_graph(int lower, double chan[], size_t i, cairo_t* cr, gboolean connect_lines)
{

  if (chan[i] > G_LIM)
    {
      chan[i] = G_LIM;
    }
  if (chan[i + 1] > G_LIM)
    {
      chan[i + 1] = G_LIM;
    }

  if (connect_lines)
    {
      connecting_lines(cr, lower, i, chan);
    }

  draw_bars_only(cr, lower, i, chan);
}

void
draw_bars_only(cairo_t* cr, int lower, size_t i, double chan[])
{

  if (chan[i + 1] > 1.0)
    {
      if (!super_bars)
        {

          if (lower)
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF), G_MAX + (double)chan[i]);
            }
          else
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF), G_MAX - (double)chan[i]);
            }

          cairo_line_to(cr, (double)((i * 4) + G_OFF), G_MAX);
          cairo_stroke(cr);
        }
      else
        {
          cairo_move_to(cr, (double)((i * 4) + G_OFF), G_MAX2 - (double)chan[i]);
          cairo_line_to(cr, (double)((i * 4) + G_OFF), G_MAX2);
          cairo_stroke(cr);
        }
    }
}

void
connecting_lines(cairo_t* cr, int lower, size_t i, double chan[])
{

  if (i < 98)
    {

      if (lower)
        {

          if (!super_bars)
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF1), G_MAX + (double)chan[i]);
              cairo_line_to(cr, (double)(((i + 1) * 4) + G_OFF), G_MAX + (double)chan[i + 1]);
            }

          else
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF1), G_MAX2 - (double)chan[i]);
              cairo_line_to(cr, (double)(((i + 1) * 4) + G_OFF), G_MAX2 - (double)chan[i + 1]);
            }
        }

      else
        {

          if (!super_bars)
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF1), G_MAX - (double)chan[i]);
              cairo_line_to(cr, (double)(((i + 1) * 4) + G_OFF), G_MAX - (double)chan[i + 1]);
            }
          else
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF1), G_MAX2 - (double)chan[i]);
              cairo_line_to(cr, (double)(((i + 1) * 4) + G_OFF), G_MAX2 - (double)chan[i + 1]);
            }
        }

      cairo_stroke(cr);
    }
}

gboolean
on_draw2_draw(GtkDrawingArea* widget, cairo_t* cr)
{
  (void)widget;

  double hor, ver, t1, len;

  if (no_meters)
    {
      return FALSE;
    }

  Lx = (lchan[0] + lchan[1]) / 2.0;
  Rx = (rchan[0] + rchan[1]) / 2.0;

  if (Lx > CLIP_LEVEL)
    {
      L_Clipping = 1;
      Lx = 99.0;
    }

  else
    {
      L_Clipping = 0;
    }

  t1 = (Lx / 100.0) * M_PI;

  if (t1 < 0.1)
    {
      MaxLM = 0.0;
    }
  else if (t1 > MaxLM)
    {
      MaxLM = t1;
    }

  else
    {
      if (MaxLM > 0.0)
        {
          MaxLM = MaxLM - MaxLM / DECAY_FACTOR;
        }
    }

  MaxL[MLr++] = t1 * t1;
  if (MLr == RMX)
    {
      MLr = 0;
    }

  double Avg = 0.0;

  for (size_t i = 0; i < RMX; i++)
    {
      Avg += MaxL[i];
    }

  if (legacy_vumeters)
    {
      hor = MHOR + 44.;
      ver = 62.0;
      len = 80.0;
      needles(cr, hor, ver, len, t1, MaxLM, 0, sqrtf(Avg / RMX));
    }
  else
    {
      hor = MHOR;
      ver = 50.0;
      len = 30.0;
      needles(cr, hor, ver, len, t1, MaxLM, 1, sqrtf(Avg / RMX));
    }

  if (legacy_vumeters)
    {
      cairo_move_to(cr, hor - 9.0, ver - 2.0);
    }
  else
    {
      cairo_move_to(cr, hor - 10.0, ver + 15.0);
    }

  if (L_Clipping)
    {
      cairo_show_text(cr, "Peak");
    }
  else
    {
      cairo_show_text(cr, "Left");
    }
  cairo_stroke(cr);

  if (Rx > CLIP_LEVEL)
    {
      R_Clipping = 1;
      Rx = 99.0;
    }

  else
    {
      R_Clipping = 0;
    }

  t1 = (Rx / 100.0) * M_PI;

  if (t1 < 0.1)
    {
      MaxRM = 0.0;
    }
  else if (t1 > MaxRM)
    {
      MaxRM = t1;
    }
  else
    {
      if (MaxRM > 0.0)
        {
          MaxRM = MaxRM - MaxRM / DECAY_FACTOR;
        }
    }

  MaxR[MRr++] = t1 * t1;
  if (MRr == RMX)
    {
      MRr = 0;
    }

  Avg = 0.0;

  for (size_t i = 0; i < RMX; i++)
    {
      Avg += MaxR[i];
    }

  if (legacy_vumeters)
    {
      hor = MHOR + 240;
      needles(cr, hor, ver, len, t1, MaxLM, 0, sqrtf(Avg / RMX));
    }
  else
    {
      hor = MHOR + 180;
      needles(cr, hor, ver, len, t1, MaxRM, 2, sqrtf(Avg / RMX));
    }

  if (legacy_vumeters)
    {
      cairo_move_to(cr, hor - 12.0, ver - 2.0);
    }
  else
    {
      cairo_move_to(cr, hor - 14.0, ver + 15.0);
    }

  if (R_Clipping)
    {
      cairo_show_text(cr, "Peak");
    }
  else
    {
      cairo_show_text(cr, "Right");
    }
  cairo_stroke(cr);

  MRx = MLx = 0.0;

  if (ML > CLIP_LEVEL)
    {
      ML = 99.0;
    }

  t1 = (ML / 100.0) * M_PI;

  if (t1 < 0.1)
    {
      MaxMM = 0.;
    }
  else if (t1 > MaxMM)
    {
      MaxMM = t1;
    }
  else
    {
      if (MaxMM > 0.0)
        {
          MaxMM = MaxMM - MaxMM / DECAY_FACTOR;
        }
    }

  MaxM[MMr++] = t1 * t1;
  if (MMr == RMX)
    {
      MMr = 0;
    }

  Avg = 0.0;

  for (size_t i = 0; i < RMX; i++)
    {
      Avg += MaxM[i];
    }

  if (legacy_vumeters)
    {
      hor = MHOR + 438;
      needles(cr, hor, ver, len, t1, MaxLM, 0, sqrtf(Avg / RMX));
    }
  else
    {
      hor = 340.0;
      needles(cr, hor, ver, len, t1, MaxMM, 3, sqrtf(Avg / RMX));
    }

  if (legacy_vumeters)
    {
      cairo_move_to(cr, hor - 30.0, ver - 2.0);
    }
  else
    {
      cairo_move_to(cr, hor - 30.0, ver + 15.0);
    }

  cairo_show_text(cr, "Microphone");
  cairo_stroke(cr);

  if (legacy_vumeters)
    {
      return FALSE;
    }

  hor = MHOR + 90;
  len = 30.0;

  Balance = 50.0 + 2.0 * (Rx - Lx);

  if (Balance > 99.0)
    {
      Balance = 99.0;
    }
  if (Balance < 0.0)
    {
      Balance = 0.0;
    }
  t1 = (Balance / 100.0) * M_PI;

  needles(cr, hor, ver, len, t1, 0, 4, 0.0);

  cairo_move_to(cr, hor - 18., ver + 15.0);
  cairo_show_text(cr, "Balance");
  cairo_stroke(cr);

  return FALSE;
  /*
  cairo_set_line_width(cr, LINE_WIDTH);

  if (L_Clipping)
    {

      cairo_set_source_rgb(cr, 1.0, 1.0, 0.3);

      cairo_move_to(cr, HOR_ORIGIN - 30, VER_ORIGIN + 3);

      cairo_show_text(cr, "Peak");
      cairo_stroke(cr);
    }

  if (R_Clipping)
    {

      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

      cairo_move_to(cr, HOR_ORIGIN - 30, VER_ORIGIN + LINE_WIDTH + LINE_SEP + 5);

      cairo_show_text(cr, "Peak");
      cairo_stroke(cr);
    }

  double left_bar;

  left_bar = BAR_WIDTH * Lx;

  double right_bar;

  right_bar = BAR_WIDTH * Rx;

  cairo_set_source_rgb(cr, 1.0, 1.0, 0.3);

  cairo_move_to(cr, HOR_ORIGIN, VER_ORIGIN);
  cairo_line_to(cr, left_bar + HOR_ORIGIN, VER_ORIGIN);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 1.0, 0.0, 0.);

  cairo_move_to(cr, HOR_ORIGIN, VER_ORIGIN + LINE_WIDTH + LINE_SEP);
  cairo_line_to(cr, right_bar + HOR_ORIGIN, VER_ORIGIN + LINE_WIDTH + LINE_SEP);
  cairo_stroke(cr);

  static double mic_bar;
  mic_bar = BAR_WIDTH * ML;

  cairo_set_line_width(cr, LINE_WIDTH);

  if (ML < 95.0)
    {
      cairo_set_source_rgb(cr, 0.5, 1.0, 0.5);
    }
  else
    {

      cairo_set_source_rgb(cr, 0.5, 1.0, 0.5);
      cairo_move_to(cr, HOR_ORIGIN - 25, VER_ORIGIN + 2 * LINE_WIDTH + 2 * LINE_SEP + 4);
      cairo_show_text(cr, "Peak");
      cairo_stroke(cr);

      cairo_set_source_rgb(cr, 0.9, 0.5, 1.0);
    }

  MR = ML = 0.0;

  cairo_move_to(cr, HOR_ORIGIN, VER_ORIGIN + 2 * LINE_WIDTH + 2 * LINE_SEP);
  cairo_line_to(cr, mic_bar + HOR_ORIGIN, VER_ORIGIN + 2 * LINE_WIDTH + 2 * LINE_SEP);
  cairo_stroke(cr);

  return FALSE;
  */
}

void
fftTobars(fftw_complex* fft, int* bars)
{

  double strength, real, imagin, scale;
  int amplitude, i;

  scale = 0.0003;

  i = 0;

  for (size_t bar = 0; bar < WindowWidth / 2; bar++)
    {

      real = fft[i][0] * scale;
      imagin = fft[i][1] * scale;
      strength = real * real + imagin * imagin;
      i++;

      if (strength < 1.00e-10)
        {
          strength = 1.00e-10;
        }

      amplitude = (maxHeight + (int)(10.0 * log10(strength))) * 2.0;

      if (amplitude > maxHeight)
        {
          amplitude = maxHeight;
        }
      if (amplitude < 0)
        {
          amplitude = 0;
        }

      bars[bar] = amplitude;

      if (spectrum_width == 1)
        {
          bar++;
          bars[bar] = amplitude;
        }
    }
}

void
spectrum()
{
  size_t size = SAMPLE_SIZE / 2;

  static float* Hann = 0;
  static double* in = 0;
  static fftw_complex* out = 0;

  in = (double*)fftw_malloc(sizeof(double) * size);
  out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * size);

  if (!Hann)
    {

      Hann = (float*)malloc(sizeof(float) * size);

      for (size_t n = 0; n < size; n++)
        {
          Hann[n] = 0.5 * (1.0 - cos(2.0 * M_PI * n / (size - 1.0)));
        }
    }

  fftw_plan plan = fftw_plan_dft_r2c_1d(size, in, out, FFTW_ESTIMATE);

  int barsL[WindowWidth / 2], barsR[WindowWidth / 2];

  for (size_t i = 0; i < size; i++)
    {
      in[i] = Hann[i] * exchangeBuf[i * 2];
    }

  fftw_execute(plan);

  fftTobars(out, barsL);

  double F = 1.0;

  for (size_t i = 0; i < WindowWidth / 2; i++)
    {
      if (barsL[i] < 0.0)
        {
          slchan[i] = 0.0;
        }
      else
        {
          if (super_bars)
            {
              barsL[i] *= F;
              F += 0.02;
              slchan[i] = barsL[i] * SCALE_BASELINE_SPECTRUM;
            }
          else
            {
              barsL[i] *= F;
              F += 0.01;
              slchan[i] = barsL[i] * SCALE_MIDLINE_SPECTRUM;
            }
          if (slchan[i] > SPECTRUM_PEAK)
            {
              slchan[i] = SPECTRUM_PEAK;
            }
        }
    }

  for (size_t i = 0; i < size; i++)
    {
      in[i] = Hann[i] * exchangeBuf[i * 2 + 1];
    }

  fftw_execute(plan);
  fftTobars(out, barsR);

  F = 1.0;
  for (size_t i = 0; i < WindowWidth / 2; i++)
    {
      if (barsR[i] < 0.0)
        {
          srchan[i] = 0.0;
        }
      else
        {
          if (super_bars)
            {
              barsR[i] *= F;
              F += 0.02;
              srchan[i] = barsR[i] * SCALE_BASELINE_SPECTRUM;
            }
          else
            {
              barsR[i] *= F;
              F += 0.01;
              srchan[i] = barsR[i] * SCALE_MIDLINE_SPECTRUM;
            }
          if (srchan[i] > SPECTRUM_PEAK)
            {
              srchan[i] = SPECTRUM_PEAK;
            }
        }
    }

  fftw_free(in);
  fftw_free(out);

  exchange = 0;
}

void
frequency_marks(cairo_t* cr)
{

  if (style == NARROW_SPECTRUM)
    {

      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

      cairo_move_to(cr, 1.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "kHz");
      cairo_stroke(cr);

      cairo_move_to(cr, 38.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "1");
      cairo_stroke(cr);

      cairo_move_to(cr, 88.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "2.5");
      cairo_stroke(cr);

      cairo_move_to(cr, 185.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "5");
      cairo_stroke(cr);

      cairo_move_to(cr, 270.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "7.5");
      cairo_stroke(cr);

      cairo_move_to(cr, 360.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "10");
      cairo_stroke(cr);

      return;
    }

  else if (style == WIDE_SPECTRUM)
    {
      cairo_set_source_rgb(cr, 1.2, 1.2, 1.0);

      cairo_move_to(cr, 1.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "kHz");
      cairo_stroke(cr);

      cairo_move_to(cr, 40.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "2.5");
      cairo_stroke(cr);

      cairo_move_to(cr, 88.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "5");
      cairo_stroke(cr);

      cairo_move_to(cr, 130.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "7.5");
      cairo_stroke(cr);

      cairo_move_to(cr, 176.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "10");
      cairo_stroke(cr);

      cairo_move_to(cr, 270.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "15");
      cairo_stroke(cr);

      cairo_move_to(cr, 360.0, SPECTRUM_FREQ_VERTICAL);
      cairo_show_text(cr, "20");
      cairo_stroke(cr);
    }
}

char*
sink_name()
{
  static char x[32] = {0};
  sprintf(x, "sink%d", getpid());
  return x;
}
char*
source_name()
{
  static char x[32] = {0};
  sprintf(x, "source%d", getpid());
  return x;
}

void
on_destroy()
{
  pa_glib_mainloop_free(m_sink);
  pa_glib_mainloop_free(m_source);
  gtk_main_quit();
}

void
show_error_sink(const char* txt)
{
  printf("%s: %s", txt, pa_strerror(pa_context_errno(context_sink)));
  on_destroy();
}

void
show_error_source(const char* txt)
{
  printf("%s: %s", txt, pa_strerror(pa_context_errno(context_source)));
  on_destroy();
}

void
stream_read_callback_sink(pa_stream* s, size_t l, void* dummy)
{
  (void)dummy;

  const void* p;

  if (pa_stream_peek(s, &p, &l) < 0)
    {
      g_message("pa_stream_peek() failed: %s", pa_strerror(pa_context_errno(context_sink)));
      return;
    }

  float* audio_data = (float*)p;
  size_t samples = l / sizeof(float);
  float levels[2] = {0.0};

  if (samples < 2)
    {
      pa_stream_drop(s);
      return;
    }

  if (samples > SAMPLE_SIZE)
    {
      for (size_t i = 0; i < SAMPLE_SIZE; i++)
        {
          exchangeBuf[i] = audio_data[i];
        }
      exchange = 1;
    }

  while (samples >= sink_channels)
    {
      for (size_t c = 0; c < sink_channels; c++)
        {
          float v = fabs(audio_data[c]);
          if (v > levels[c])
            {
              levels[c] = v;
            }
        }
      audio_data += sink_channels;
      samples -= sink_channels;
    }

  if (LeftChan < levels[0])
    {
      LeftChan = levels[0];
    }
  if (RightChan < levels[1])
    {
      RightChan = levels[1];
    }

  pa_stream_drop(s);
}

void
stream_read_callback_source(pa_stream* s, size_t len, void* user)
{
  (void)user;
  const void* p;

  if (no_microphone)
    {
      pa_stream_drop(s);
    }

  if (pa_stream_peek(s, &p, &len) < 0)
    {
      g_message("pa_stream_peek() failed: %s", pa_strerror(pa_context_errno(context_source)));
      return;
    }

  float* pcm = (float*)p;
  size_t samples = len / sizeof(float);
  float levels[2] = {0.0};

  if (samples < 2)
    {
      pa_stream_drop(s);
      return;
    }

  while (samples >= source_channels)
    {
      for (size_t c = 0; c < source_channels; c++)
        {
          float v = fabs(pcm[c]);
          if (v > levels[c])
            {
              levels[c] = v;
            }
        }
      pcm += source_channels;
      samples -= source_channels;
    }

  if (MLx < levels[0])
    {
      MLx = levels[0];
    }
  if (MRx < levels[0])
    {
      MRx = levels[1];
    }

  ML = MLx;
  MR = MRx;

  if (source_channels == 2)
    {
      ML = (ML + MR) / 2;
    }
  else
    {
      MR = ML;
    }

  ML = log10(ML + 1) * (FACTOR - 20.0);

  pa_stream_drop(s);
}

void
stream_state_callback_sink(pa_stream* s, void* dummy)
{
  (void)dummy;

  switch (pa_stream_get_state(s))
    {

    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:

      break;

    case PA_STREAM_READY:

      break;

    case PA_STREAM_FAILED:

      show_error_sink("Connection failed");
      break;

    case PA_STREAM_TERMINATED:

      pa_context_disconnect(context_sink);
      pa_stream_unref(stream_sink);
      on_destroy();
    }
}

void
stream_state_callback_source(pa_stream* s, void* dummy)
{
  (void)dummy;

  switch (pa_stream_get_state(s))
    {

    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:

      break;

    case PA_STREAM_READY:

      break;

    case PA_STREAM_FAILED:

      show_error_source("Connection failed");
      break;

    case PA_STREAM_TERMINATED:

      pa_context_disconnect(context_source);
      pa_stream_unref(stream_source);
      on_destroy();
    }
}

void
sink_create_stream(const char* name, const char* description,
                   const pa_sample_spec sampSpec, const pa_channel_map cmap)
{
  (void)description;

  char txt[256] = {0};
  pa_sample_spec nss;

  g_free(device_name_sink);
  device_name_sink = g_strdup(device_name_sink);

  g_free(device_description_sink);
  device_description_sink = g_strdup(device_description_sink);

  nss.format = PA_SAMPLE_FLOAT32;
  nss.rate = sampSpec.rate;
  nss.channels = sampSpec.channels;

  printf("\nPlayback Sample format for device %s:\n\t%s\n",
         device_name_sink, pa_sample_spec_snprint(txt, sizeof(txt), &nss));

  printf("Playback Create stream Channel map for device %s:\n\t%s\n",
         device_name_sink, pa_channel_map_snprint(txt, sizeof(txt), &cmap));

  stream_sink = pa_stream_new(context_sink, PULSE_SINK_NAME, &nss, &cmap);

  pa_stream_set_state_callback(stream_sink, stream_state_callback_sink, 0);

  pa_stream_set_read_callback(stream_sink, stream_read_callback_sink, 0);

  pa_buffer_attr paba_sink = {-1, (SAMPLE_SIZE)*4, -1, SAMPLE_SIZE * 4, -1};

  int sink_err = pa_stream_connect_record(stream_sink, name, &paba_sink,
                                          (enum pa_stream_flags)PA_STREAM_ADJUST_LATENCY);

  if (sink_err)
    {
      printf("*** stream connect error\n");
    }
}

void
source_create_stream(const char* name, const char* description,
                     const pa_sample_spec sampSpec, const pa_channel_map cmap)
{
  (void)description;

  char txt[256] = {0};
  pa_sample_spec nss;

  g_free(device_name_source);
  device_name_source = g_strdup(device_name_source);

  g_free(device_description_source);
  device_description_source = g_strdup(device_description_source);

  nss.format = PA_SAMPLE_FLOAT32;
  nss.rate = sampSpec.rate;
  nss.channels = sampSpec.channels;

  printf("\n+++ source Sample format for device %s\n",
         pa_sample_spec_snprint(txt, sizeof(txt), &nss));

  printf("+++ source Create stream Channel map %s\n",
         pa_channel_map_snprint(txt, sizeof(txt), &cmap));

  stream_source = pa_stream_new(context_source, PULSE_SOURCE_NAME, &nss, &cmap);

  pa_stream_set_state_callback(stream_source, stream_state_callback_source, 0);

  pa_stream_set_read_callback(stream_source, stream_read_callback_source, 0);

  pa_buffer_attr paba_sink = {4000, -1, -1, -1, 4000};

  pa_stream_connect_record(stream_source, name, &paba_sink, (enum pa_stream_flags)PA_STREAM_ADJUST_LATENCY);
}

void
source_info_callback(pa_context* p, const pa_source_info* si, int is_last, void* dummy)
{
  (void)dummy;

  if (is_last < 0)
    {
      if (p == context_sink)
        {
          show_error_sink("Failed to get sink information");
        }
      else
        {
          show_error_source("Failed to get source information");
        }
      return;
    }

  if (!si)
    {
      return;
    }

  char txt[255] = {0};

  source_channels = si->channel_map.channels;

  printf("++ channel map for device: %s\n",
         pa_channel_map_snprint(txt, sizeof(txt), &si->channel_map));

  printf("+++ name: %s\n", si->name);
  printf("+++ index: %d\n", si->index);

  printf("+++ n_ports: %d\n", si->n_ports);
  printf("\n");

  if (si->n_ports > 0)
    {
      for (size_t i = 0; i < si->n_ports; i++)
        {
          printf("+++ pa_source_port_info name %ld: %s\n", i, si->ports[i]->name);
          printf("+++ pa_source_port_info priority %ld: %d\n", i, si->ports[i]->priority);
          printf("+++ pa_source_port_info avail %ld: %d\n", i, si->ports[i]->available == PA_PORT_AVAILABLE_YES);
        }
      printf("+++ pa_source_port_info active port name: %s\n", si->active_port->name);
      printf("\n");
      printf("+++ source channels %ld\n", source_channels);
      printf("+++ map to name: %s\n", pa_channel_map_to_name(&si->channel_map));
      printf("+++ description: %s\n", si->description);
    }

  source_create_stream(si->name, si->description, si->sample_spec, si->channel_map);
}

void
sink_info_callback(pa_context* p, const pa_sink_info* si, int is_last, void* dummy)
{
  (void)dummy;

  if (is_last < 0)
    {
      if (p == context_sink)
        {
          show_error_sink("Failed to get sink information");
        }
      else
        {
          show_error_source("Failed to get source information");
        }
      return;
    }

  if (!si)
    {
      return;
    }

  sink_channels = si->channel_map.channels;

  printf("sink channels %ld\n", sink_channels);

  sink_create_stream(si->monitor_source_name, si->description, si->sample_spec, si->channel_map);
}

void
sink_server_info_callback(pa_context* c, const pa_server_info* si, void* dummy)
{
  (void)dummy;

  pa_operation* p1;

  if (!si)
    {
      if (c == context_sink)
        {
          show_error_sink("Failed to get sink information");
        }
      else
        {
          show_error_source("Failed to get source information");
        }
      return;
    }

  strcpy(default_sink, si->default_sink_name);
  strcpy(default_source, si->default_source_name);

  printf("Server info callback name of default source:\n\t%s\n", default_source);
  printf("Server info callback name of default sink:\n\t%s\n", default_sink);

  if (!si->default_sink_name)
    {
      show_error_sink("No default sink set.");
      return;
    }

  p1 = pa_context_get_sink_info_by_name(
    c,
    si->default_sink_name,
    sink_info_callback,
    0);

  pa_operation_unref(p1);
}

void
source_server_info_callback(pa_context* c, const pa_server_info* si, void* dummy)
{
  (void)dummy;

  pa_operation* p1;

  if (!si)
    {
      if (c == context_sink)
        {
          show_error_sink("Failed to get sink information");
        }
      else
        {
          show_error_source("Failed to get source information");
        }
      return;
    }

  strcpy(default_sink, si->default_sink_name);
  strcpy(default_source, si->default_source_name);

  printf("Server info callback name of default source:\n\t%s\n", default_source);
  printf("Server info callback name of default sink:\n\t%s\n", default_sink);

  printf("------- acquire microphone\n");

  if (!si->default_source_name)
    {
      show_error_source("No default source set.");
      return;
    }

  printf("default source name =  %s\n", si->default_source_name);
  if (strncmp(si->default_source_name, "alsa_input", 10) == 0 && strncmp(si->default_source_name, "RDPSource", 9) == 0)
    {
      printf("*** no microphone detected\n");
      no_microphone = 1;
      return;
    }

  printf("*** microphone found\n");
  no_microphone = 0;

  printf("default source name %s\n", si->default_source_name);
  p1 = pa_context_get_source_info_by_name(c, si->default_source_name, source_info_callback, 0);

  pa_operation_unref(p1);
}

void
source_connection_state_callback(pa_context* c, void* dummy)
{

  pa_operation* p1;

  switch (pa_context_get_state(c))
    {

    case PA_CONTEXT_UNCONNECTED:
      printf("+++++ source unconnected\n");
      break;

    case PA_CONTEXT_CONNECTING:
      printf("+++++ source connecting\n");
      break;

    case PA_CONTEXT_AUTHORIZING:
      printf("+++++ source authorizing\n");
      break;

    case PA_CONTEXT_SETTING_NAME:
      printf("+++++ source setting name\n");
      break;

    case PA_CONTEXT_READY:

      p1 = pa_context_get_server_info(c, source_server_info_callback, dummy);
      pa_operation_unref(p1);
      break;

    case PA_CONTEXT_FAILED:

      if (c == context_sink)
        {
          show_error_sink("Connection failed.");
        }
      break;

    case PA_CONTEXT_TERMINATED:
      on_destroy();
      break;

    default:
      printf("+++++ default\n");
    }
}

void
sink_connection_state_callback(pa_context* c, void* dummy)
{
  (void)dummy;

  pa_operation* p1;

  switch (pa_context_get_state(c))
    {

    case PA_CONTEXT_UNCONNECTED:
      printf("+++++ sink unconnected\n");
      break;

    case PA_CONTEXT_CONNECTING:
      printf("+++++ sink connecting\n");
      break;

    case PA_CONTEXT_AUTHORIZING:
      printf("+++++ sink authorizing\n");
      break;

    case PA_CONTEXT_SETTING_NAME:
      printf("+++++ sink setting name\n");
      break;

    case PA_CONTEXT_READY:

      p1 = pa_context_get_server_info(
        c,
        sink_server_info_callback,
        0);

      pa_operation_unref(p1);

      break;

    case PA_CONTEXT_FAILED:

      if (c == context_sink)
        {
          show_error_sink("Connection failed.");
        }
      else
        {
          show_error_source("Connection failed.");
        }
      break;

    case PA_CONTEXT_TERMINATED:
      on_destroy();
      break;

    default:
      printf("+++++ default\n");
    }
}

int
main(int argc, char* argv[])
{

  signal(SIGPIPE, SIG_IGN);

  gtk_init(&argc, &argv);

  printf("\n-----------------------------------------------\n");

  builder = gtk_builder_new_from_resource("/part1/part1.glade");

  window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

  g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), 0);

  gtk_builder_connect_signals(builder, 0);

  fixed1 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed1"));
  Bars = GTK_WIDGET(gtk_builder_get_object(builder, "Bars"));
  draw1 = GTK_WIDGET(gtk_builder_get_object(builder, "draw1"));
  draw2 = GTK_WIDGET(gtk_builder_get_object(builder, "draw2"));
  Spectrum = GTK_WIDGET(gtk_builder_get_object(builder, "Spectrum"));
  meters = GTK_WIDGET(gtk_builder_get_object(builder, "meters"));
  balance = GTK_WIDGET(gtk_builder_get_object(builder, "balance"));
  vumeter1 = GTK_WIDGET(gtk_builder_get_object(builder, "vumeter1"));
  vumeter2 = GTK_WIDGET(gtk_builder_get_object(builder, "vumeter2"));
  vumeter3 = GTK_WIDGET(gtk_builder_get_object(builder, "vumeter3"));

  g_object_unref(builder);

  gtk_widget_show(window);

  GtkCssProvider* css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(css_provider, "* { background-color: #000000; }", -1, 0);
  GtkStyleContext* style_context = gtk_widget_get_style_context(window);
  gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

  char cmd[1024] = {0};
  strcpy(cmd, "Spark Gap Radio Meters 2.1");
  gtk_window_set_title(GTK_WINDOW(window), cmd);

  style = NARROW_SPECTRUM;

  int opt;

  while ((opt = getopt(argc, argv, "abcglmBwh?vs:")) != -1)
    {

      switch (opt)
        {

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
          gtk_window_resize(GTK_WINDOW(window), 20, 20);
          strcpy(cmd, "Spark Gap Radio Engineering Dept Spectrum 2.00");
          gtk_window_set_title(GTK_WINDOW(window), cmd);
          break;

        case 'l':
          connect_graph = TRUE;
          break;

        case 'm':
          no_graph = TRUE;
          gtk_fixed_move(
            fixed1,
            draw2,
            1, 20);
          gtk_window_resize(GTK_WINDOW(window), 20, 20);
          strcpy(cmd, "Spark Gap Radio Engineering Dept VU Meters 2.00");
          gtk_window_set_title(GTK_WINDOW(window), cmd);
          break;

        case 'B':
          connect_graph = TRUE;
          break;

        case 'w':
          style = WIDE_SPECTRUM;
          break;

        case 'v':
          no_graph = TRUE;
          legacy_vumeters = 1;
          gtk_widget_hide(draw1);
          gtk_fixed_move(
            fixed1,
            draw2,
            1, 20);
          gtk_widget_set_size_request(draw2, 580, 74);
          gtk_window_resize(GTK_WINDOW(window), 20, 20);
          strcpy(cmd, "Spark Gap Radio Engineering Dept VU Meters 2.00");
          gtk_window_set_title(GTK_WINDOW(window), cmd);
          break;

        case 's':
          timer_res = atoi(optarg);
          break;

        case 'h':
        case '?':
        default:
          fprintf(stderr, "Usage: -ablBwh?s\n");
          fprintf(stderr, "-a	show arcs\n");
          fprintf(stderr, "-b	show bars only\n");
          fprintf(stderr, "-c	centered spectrum\n");
          fprintf(stderr, "-g	spectrum graph only - no meters\n");
          fprintf(stderr, "-l	lines only\n");
          fprintf(stderr, "-m	meters only - no spectrum graph\n");
          fprintf(stderr, "-B	bars and lines\n");
          fprintf(stderr, "-w	wide spectrum\n");
          fprintf(stderr, "-v	vu meters\n");
          fprintf(stderr, "-s val	sample rate [50]\n");
          fprintf(stderr, "-h	help\n");
          fprintf(stderr, "-?	help\n");
          exit(EXIT_FAILURE);
        }
    }

  if (!legacy_vumeters)
    {
      gtk_widget_hide(vumeter1);
      gtk_widget_hide(vumeter2);
      gtk_widget_hide(vumeter3);
      gtk_window_resize(GTK_WINDOW(window), 20, 20);
    }

  printf("\n------------------ sink connect --------\n");

  GMainContext* mcd_sink = 0;
  pa_mainloop_api* mla_sink = 0;

  mcd_sink = g_main_context_default();
  m_sink = pa_glib_mainloop_new(mcd_sink);
  mla_sink = pa_glib_mainloop_get_api(m_sink);
  context_sink = pa_context_new(mla_sink, PULSE_SINK_NAME);
  pa_context_set_state_callback(context_sink, sink_connection_state_callback, 0);

  if (pa_context_connect(context_sink, 0, PA_CONTEXT_NOAUTOSPAWN, 0) < 0)
    {
      printf("*** sink context connect error\n");
    }
  printf("*** sink connected\n");

  printf("\n------------------ source connect --------\n");

  GMainContext* mcd_source = 0;
  pa_mainloop_api* mla_source = 0;

  mcd_source = g_main_context_default();
  m_source = pa_glib_mainloop_new(mcd_source);
  mla_source = pa_glib_mainloop_get_api(m_source);
  context_source = pa_context_new(mla_source, PULSE_SOURCE_NAME);
  pa_context_set_state_callback(context_source, source_connection_state_callback, 0);

  if (pa_context_connect(context_source, 0, PA_CONTEXT_NOAUTOSPAWN, 0) < 0)
    {
      printf("*** source context connect error\n");
    }
  printf("*** source connected\n");

  g_timeout_add(timer_res, (GSourceFunc)pulse_timer_handler, 0);

  gtk_main();

  return EXIT_SUCCESS;
}
