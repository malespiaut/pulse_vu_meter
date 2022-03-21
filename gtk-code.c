/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+
#+     Glade / Gtk Programming
#+
#+     Copyright (C) 2020, 2021 by Kevin C. O'Kane
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

// August 21, 2021

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "functions.h"

#include "global-extern-variables.h"

//--------------------------------------------------------------------
//	main timer process
//	called every 100 milliseconds to update the meteres and graphs.
//--------------------------------------------------------------------

gboolean
pulse_timer_handler(gpointer user_data)
{ // check pulse every 100 millisecs

  (void)user_data;

  rUtil = log10(RightChan + 1) * FACTOR;
  lUtil = log10(LeftChan + 1) * FACTOR;

  LeftChan = RightChan = 0.0;

  //----------------------
  //	shift the data
  //----------------------

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

  //----------------------------------------------
  //	average the past 3 to dampen movement
  //----------------------------------------------

  //	lchan[0] = (lchan[0] + lchan[1] + lchan[2]) / 3.0;
  //	rchan[0] = (rchan[0] + rchan[1] + rchan[2]) / 3.0;

  //--------------------------
  //	que draw events
  //--------------------------

  gtk_widget_queue_draw(draw1);
  gtk_widget_queue_draw(draw2);
  return TRUE;
}

//--------------------------
//	draw meters
//--------------------------

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

  //------------------------------------------------------------------------------
  //	color is same as needle until past needle point and then color is gray
  //------------------------------------------------------------------------------

  X = 0.0;

  if (!legacy_vumeters)
    {

      if (cFlg == 0)
        {
          cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // black
        }
      else if (cFlg == 1)
        {
          cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); // yellow
        }
      else if (cFlg == 2)
        {
          cairo_set_source_rgb(cr, 1.0, 0., 0.); // red
        }
      else if (cFlg == 3)
        {
          cairo_set_source_rgb(cr, 0.5, 1.0, 0.5); // green
        }
      else if (cFlg == 4)
        {
          cairo_set_source_rgb(cr, 0.5, 0.5, 0.5); // gray
        }

      cairo_set_line_width(cr, 2.5);

      //--------------------------------------------
      //	draw all short and long hash marks
      //--------------------------------------------

      for (size_t i = 0; i < 21; i++)
        {

          //-------------------------------
          //		long hash marks
          //-------------------------------

          if (!(i % 2))
            {

              cairo_arc(cr, hor, ver, len + 10, -M_PI, -M_PI + X); // further point
              cairo_get_current_point(cr, &x, &y);

              cairo_arc(cr, hor, ver, len, -M_PI, -M_PI + X); // nearer point
              cairo_get_current_point(cr, &x1, &y1);

              cairo_new_path(cr);
              cairo_move_to(cr, x, y);
              cairo_line_to(cr, x1, y1);
              cairo_stroke(cr);
            }

          //-----------------------------------
          //		short hash marks
          //-----------------------------------

          else
            {
              cairo_arc(cr, hor, ver, len + 2, -M_PI, -M_PI + X); // further point
              cairo_get_current_point(cr, &x, &y);

              cairo_arc(cr, hor, ver, len - 1, -M_PI, -M_PI + X); // nearer point length
              cairo_get_current_point(cr, &x1, &y1);
            }

          if (X > t1)
            { // are we past the level point?

              //--------------------------------------------------------------
              //			no more colored short hash marks
              //--------------------------------------------------------------

              cairo_set_source_rgb(cr, 0.5, 0.5, 0.5); // gray
              cairo_new_path(cr);
              cairo_move_to(cr, x, y);
              cairo_line_to(cr, x1, y1);
              cairo_stroke(cr);

              //----------------------------------------------------------------------------
              //			short marks from here only for balance
              //----------------------------------------------------------------------------

              if (cFlg != 4)
                {
                  X = X + M_PI / 20.0; // keep incrementing
                  continue;
                }
            }

          //-----------------------------------------------------------------
          //		draw a short hash mark using numbers from above.
          //-----------------------------------------------------------------

          cairo_new_path(cr);
          cairo_move_to(cr, x, y);
          cairo_line_to(cr, x1, y1);
          cairo_stroke(cr);

          X = X + M_PI / 20.0; // advance point on arc
        }

      //----------------------------
      //	RMS level
      //----------------------------

      if (Max > 0.0)
        {

          cairo_set_line_width(cr, 5.0);

          if (Max > M_PI)
            {
              Max = M_PI;
            }

          if (cFlg == 1)
            {
              cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); // yellow
            }
          else if (cFlg == 2)
            {
              cairo_set_source_rgb(cr, 1.0, 0., 0.); // red
            }
          else if (cFlg == 3)
            {
              cairo_set_source_rgb(cr, 0.5, 1.0, 0.5); // green
            }
          else if (cFlg == 4)
            {
              cairo_set_source_rgb(cr, 0.5, 0.5, 0.5); // gray
            }

          cairo_arc(cr, hor, ver, len + 10, -M_PI, -M_PI + Max); // further point
          cairo_get_current_point(cr, &x, &y);

          cairo_arc(cr, hor, ver, len + 16, -M_PI, -M_PI + Max); // nearer point
          cairo_get_current_point(cr, &x1, &y1);

          cairo_new_path(cr);
          cairo_move_to(cr, x, y);
          cairo_line_to(cr, x1, y1);
          cairo_stroke(cr);
        }

      //--------------------
      //	Max level
      //--------------------

      if (t2 > 0.0)
        {
          cairo_set_line_width(cr, 2.5);

          if (t2 > M_PI)
            {
              t2 = M_PI;
            }

          if (cFlg == 1)
            {
              cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); // yellow
            }
          else if (cFlg == 2)
            {
              cairo_set_source_rgb(cr, 1.0, 0., 0.); // red
            }
          else if (cFlg == 3)
            {
              cairo_set_source_rgb(cr, 0.5, 1.0, 0.5); // green
            }
          else if (cFlg == 4)
            {
              cairo_set_source_rgb(cr, 0.5, 0.5, 0.5); // gray
            }

          cairo_arc(cr, hor, ver, len + 10, -M_PI, -M_PI + t2); // further point
          cairo_get_current_point(cr, &x, &y);

          cairo_arc(cr, hor, ver, len + 16, -M_PI, -M_PI + t2); // nearer point
          cairo_get_current_point(cr, &x1, &y1);

          cairo_new_path(cr);
          cairo_move_to(cr, x, y);
          cairo_line_to(cr, x1, y1);
          cairo_stroke(cr);
        }
    }

  //----------------
  //	draw needle
  //----------------

  cairo_set_line_width(cr, 2.5);

  if (cFlg == 0)
    {
      cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // black
    }
  else if (cFlg == 1)
    {
      cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); // yellow
    }
  else if (cFlg == 2)
    {
      cairo_set_source_rgb(cr, 1.0, 0., 0.); // red
    }
  else if (cFlg == 3)
    {
      cairo_set_source_rgb(cr, 0.5, 1.0, 0.5); // green
    }
  else if (cFlg == 4)
    {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5); // black
    }

  cairo_set_line_width(cr, 3.5);

  //-------------------------------------
  //	arc from left horizon to t1
  //-------------------------------------

  cairo_arc(cr, hor, ver, len, -M_PI, -M_PI + t1);

  //----------------------------------------
  //	get coordinates of point on arc
  //----------------------------------------

  cairo_get_current_point(cr, &x, &y);

  //-----------------------------------------------
  //	draw a line from the origin to the point
  //-----------------------------------------------

  cairo_new_path(cr);
  cairo_move_to(cr, x, y);
  cairo_line_to(cr, hor, ver);
  cairo_stroke(cr);

  if (legacy_vumeters)
    {
      return;
    }

  //--------------
  //	arcs
  //--------------

  if (cFlg == 4)
    { // balance meter - always show arc

      //------------------------------------------------------------------------
      //		leans left - M_PI_2 is PI/2 -> -M_PI_2 is center of arc
      //------------------------------------------------------------------------

      if (t1 < M_PI_2)
        {

          cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); // yellow

          //------------------------------------------
          //			from left to center
          //------------------------------------------

          cairo_arc(cr, hor, ver, len - 2, -M_PI + t1, -M_PI_2); // left to center
          cairo_line_to(cr, hor, ver);
          cairo_fill(cr); // fill in arc
          cairo_stroke(cr);
        }

      //------------------------------------------------------------------------
      //		leans right - M_PI_2 is PI/2 -> -M_PI_2 is center of arc
      //------------------------------------------------------------------------

      else
        {
          cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); // red

          //------------------------------------------
          //			from center to right
          //------------------------------------------

          //--------------------------------------------------------------------
          //			following adds line from current point to arc as well.
          //			current point is the last cairo_line_to above (the origin)
          //--------------------------------------------------------------------

          cairo_arc(cr, hor, ver, len - 2, -M_PI_2, -M_PI + t1); // center to right
          cairo_line_to(cr, hor, ver);
          cairo_fill(cr); // fill in arc
          cairo_stroke(cr);
        }

      cairo_set_source_rgb(cr, 0.6, 0.6, 0.6); // gray
    }

  //-----------------------------
  //	other arcs is enabled
  //-----------------------------

  if (cFlg != 4 && showArc)
    {

      //--------------------------------------------
      //		draw arc from left horizon to t1
      //--------------------------------------------

      //--------------------------------------------------------------------
      //		following adds line from current point to arc as well.
      //		current point is the last cairo_line_to above (the origin)
      //--------------------------------------------------------------------

      cairo_arc(cr, hor, ver, len - 2, -M_PI, -M_PI + t1);
      cairo_line_to(cr, hor, ver);
      cairo_fill(cr); // fill in arc
      cairo_stroke(cr);
    }

  //-------------------------
  //	spindle - full arc
  //-------------------------

  //--------------------------------------------------------------------
  //	following adds line from current point to arc as well.
  //	current point is the last cairo_line_to above (the origin)
  //--------------------------------------------------------------------

  cairo_arc(cr, hor, ver, 4.0, -M_PI, M_PI);
  cairo_line_to(cr, hor, ver);
  cairo_fill(cr); // fill in arc
}

#define DRAW_HEIGHT 105.0

float UPPER_MAX = DRAW_HEIGHT - DRAW_HEIGHT / 10.0;
float UPPER_MAX_MAX = DRAW_HEIGHT - DRAW_HEIGHT / 20.0;

#define SPECTRUM_PEAK DRAW_HEIGHT - 30.0 // maximum graph level for freq spectrum (clamp level)`

#define SCALE_BASELINE_SPECTRUM DRAW_HEIGHT / 160.0
#define SCALE_MIDLINE_SPECTRUM DRAW_HEIGHT / 250.0

#define SCALE_BASELINE_VOLUME DRAW_HEIGHT / 150.0

//	colors of warning level limits

#define UPPER_MAX_MAX_COLOR cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
#define UPPER_MAX_COLOR cairo_set_source_rgb(cr, 1.0, 0.7, 0.7);

#define RED 1
#define YELLOW 2
#define WHITE 3
#define G_OFF 4
#define G_OFF1 4 // bar horizontal offset
#define UPPER 0
#define LOWER 1

#define BAR_WIDTH1 2.9

//----------------------------------------------------
//
// draw frequency spectrum or volume level graphs
//
//----------------------------------------------------

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

      //------------------------
      //	frequency graph
      //------------------------

      double blchan[100] = {NAN};
      double brchan[100] = {NAN};

      for (size_t i = 0; i < 100; i++)
        {
          blchan[i] = slchan[i];
          brchan[i] = srchan[i];
        }

      spectrum(); // scaling is done in spectrum()
      noLine = 1;

      for (size_t i = 0; i < 99; i++)
        { // make bars less jumpy

          slchan[i] = (slchan[i] + blchan[i]) / 2.0; // left
          srchan[i] = (srchan[i] + brchan[i]) / 2.0; // right
        }
    }

  //---------------------
  //	volume graph
  //---------------------

  else
    { // volume graph
      if (super_bars)
        {
          for (size_t i = 0; i < 100; i++)
            { // scale and segregate data
              slchan[i] = lchan[i] * SCALE_BASELINE_VOLUME;
              srchan[i] = rchan[i] * SCALE_BASELINE_VOLUME;
            }
        }
      else
        {
          for (size_t i = 0; i < 100; i++)
            { // segregate data unscaled
              slchan[i] = lchan[i];
              srchan[i] = rchan[i];
            }
        }
    }

  //------------------------
  //	width of bars
  //------------------------

  cairo_set_line_width(cr, BAR_WIDTH1);

  for (size_t i = 0; i < 99; i++)
    { // draw graph

      if (!connect_graph && super_bars)
        {

          if (slchan[i] > srchan[i])
            {                                                 // show prominent color
              color_bars(cr, i, noLine, YELLOW);              // left channel color
              bar_graph(UPPER, slchan, i, cr, connect_graph); // draw left channel only
            }
          else
            {
              color_bars(cr, i, noLine, RED);                 // right channel color
              bar_graph(LOWER, srchan, i, cr, connect_graph); // draw right channel only
            }
        }

      else
        {

          color_bars(cr, i, noLine, YELLOW); // left channel
          bar_graph(UPPER, slchan, i, cr, connect_graph);

          color_bars(cr, i, noLine, RED); // right channel
          bar_graph(LOWER, srchan, i, cr, connect_graph);
        }
    }

  //-------------------------------------------------
  //	midpoint line - none if not centered graph
  //-------------------------------------------------

  if (!super_bars)
    { // mid point line of centered graph
      cairo_set_line_width(cr, 1.0);
      cairo_set_source_rgb(cr, 0.0, 1.0, 0.0); // green
      cairo_move_to(cr, (double)G_OFF - 2, DRAW_HEIGHT / 2.0 - 10.0);
      cairo_line_to(cr, (double)G_OFF + 394, DRAW_HEIGHT / 2.0 - 10.0);
      cairo_stroke(cr);
    }

  //---------------------------------
  //	frequency legend marks
  //---------------------------------

  frequency_marks(cr);

  //-----------------------------------------------------------------------------
  //	for future reference - not currently used
  //	guint width, height;
  //	width = gtk_widget_get_allocated_width (widget);   // of draw window
  //	height = gtk_widget_get_allocated_height (widget); // of draw window
  //-----------------------------------------------------------------------------
  return TRUE;
}

//--------------------------------
//	Select color for bars
//--------------------------------

void
color_bars(cairo_t* cr, int i, int noLine, int color)
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
          cairo_set_source_rgb(cr, 1.0, 0.1, 0.1); // red
        }
      else if (color == YELLOW)
        {
          cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); // yellow
        }
      else if (color == WHITE)
        {
          cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // white
        }
      else
        {
          cairo_set_source_rgb(cr, 0.2, 0.2, 1.0); // blue
        }
    }

  //--------------------------------------------------------------
  //	create a color from deep red up to violet depending on i
  //--------------------------------------------------------------

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

#define G_MAX DRAW_HEIGHT / 2.0 - 10.0 // graph center line location (zero is top)
#define G_MAX2 DRAW_HEIGHT - 20        // graph center line location (zero is top)
#define G_LIM DRAW_HEIGHT              // bar length limit

//------------------------------------------
//	Process line (i) of the bar graph
//------------------------------------------

void
bar_graph(int lower, double chan[], int i, cairo_t* cr, gboolean connect_lines)
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
      connecting_lines(cr, lower, i, chan); // connecting horizontal graph lines
    }

  draw_bars_only(cr, lower, i, chan); // vertical bars
}

//-----------------------------
//	draw vertical bars
//-----------------------------

void
draw_bars_only(cairo_t* cr, int lower, int i, double chan[])
{

  if (chan[i + 1] > 1.0)
    {
      if (!super_bars)
        { // centered

          if (lower)
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF), G_MAX + (double)chan[i]);
            }
          else
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF), G_MAX - (double)chan[i]);
            }

          cairo_line_to(cr, (double)((i * 4) + G_OFF), G_MAX); // baseline line
          cairo_stroke(cr);
        }
      else
        {                                                                         // not centered
          cairo_move_to(cr, (double)((i * 4) + G_OFF), G_MAX2 - (double)chan[i]); // horizontal, vertical
          cairo_line_to(cr, (double)((i * 4) + G_OFF), G_MAX2);                   // baseline line
          cairo_stroke(cr);
        }
    }
}

//-------------------------------------------------
//	draw connecting horizontal graph lines
//-------------------------------------------------

void
connecting_lines(cairo_t* cr, int lower, int i, double chan[])
{

  if (i < 98)
    { // not at the end of the graph

      if (lower)
        { // below the centerline

          if (!super_bars)
            {                                                                                  // center line graph
              cairo_move_to(cr, (double)((i * 4) + G_OFF1), G_MAX + (double)chan[i]);          // first bar
              cairo_line_to(cr, (double)(((i + 1) * 4) + G_OFF), G_MAX + (double)chan[i + 1]); // second bar
            }

          else
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF1), G_MAX2 - (double)chan[i]);          // first bar
              cairo_line_to(cr, (double)(((i + 1) * 4) + G_OFF), G_MAX2 - (double)chan[i + 1]); // second bar
            }
        }

      else
        { // above the center line

          if (!super_bars)
            {
              cairo_move_to(cr, (double)((i * 4) + G_OFF1), G_MAX - (double)chan[i]);          // first
              cairo_line_to(cr, (double)(((i + 1) * 4) + G_OFF), G_MAX - (double)chan[i + 1]); // second
            }
          else
            {                                                                                   // not center line
              cairo_move_to(cr, (double)((i * 4) + G_OFF1), G_MAX2 - (double)chan[i]);          // first
              cairo_line_to(cr, (double)(((i + 1) * 4) + G_OFF), G_MAX2 - (double)chan[i + 1]); // second
            }
        }

      cairo_stroke(cr); // do it
    }
}

//-----------------------------
//
//	draw meters and bars
//
//-----------------------------

gboolean
on_draw2_draw(GtkDrawingArea* widget, cairo_t* cr)
{
  (void)widget;

  double hor, ver, t1, len;

  if (no_meters)
    {
      return FALSE;
    }

#define MHOR 55.0

  //----------------------------------------------
  //	average the past X to dampen movement
  //----------------------------------------------

  //	Lx = (lchan[0] + lchan[1] + lchan[2] + lchan[3] ) / 4.0;
  //	Rx = (rchan[0] + rchan[1] + rchan[2] + rchan[3] ) / 4.0;

  //	Lx = (lchan[0] + lchan[1] + lchan[2] ) / 3.0;
  //	Rx = (rchan[0] + rchan[1] + rchan[2] ) / 3.0;

  Lx = (lchan[0] + lchan[1]) / 2.0;
  Rx = (rchan[0] + rchan[1]) / 2.0;

  //	Lx = lchan[0];
  //	Rx = rchan[0];

#define DECAY_FACTOR 250.0

  //--------------------------------
  //	LEFT Channel meter
  //--------------------------------

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

  //.............................

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

  //------------------------------------------------------
  //	position of origin point and length of needle:
  //------------------------------------------------------

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
      cairo_move_to(cr, hor - 9.0, ver - 2.0); // top
    }
  else
    {
      cairo_move_to(cr, hor - 10.0, ver + 15.0); // top
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

  //--------------------------------
  //	RIGHT Channel meter
  //--------------------------------

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
      MaxRM = 0.;
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
      cairo_move_to(cr, hor - 12.0, ver - 2.0); // top
    }
  else
    {
      cairo_move_to(cr, hor - 14.0, ver + 15.0); // top
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

  //----------------------------------------------------------
  //	microphone meter
  //----------------------------------------------------------

  MRx = MLx = 0.0; // start a new sample - this one has been used.

  if (ML > CLIP_LEVEL)
    {
      ML = 99.0; // limit
    }

  t1 = (ML / 100.0) * M_PI; // fraction of PI

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
      cairo_move_to(cr, hor - 30.0, ver - 2.0); // top
    }
  else
    {
      cairo_move_to(cr, hor - 30.0, ver + 15.0); // top
    }

  cairo_show_text(cr, "Microphone");
  cairo_stroke(cr);

  //--------------------------
  // 	balance meter
  //--------------------------

  if (legacy_vumeters)
    {
      return FALSE;
    }

  hor = MHOR + 90;
  len = 30.0;

  Balance = 50.0 + 2.0 * (Rx - Lx); // difference & double the effect

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

  cairo_move_to(cr, hor - 18., ver + 15.0); // top
  cairo_show_text(cr, "Balance");
  cairo_stroke(cr);

#define HOR_ORIGIN 32.0
#define VER_ORIGIN 78.0
#define BAR_WIDTH 3.25
#define LINE_WIDTH 5
#define LINE_SEP 4.0

  return FALSE; // NO BARS
  cairo_set_line_width(cr, LINE_WIDTH);

  //-------------------------
  //	media clipping
  //-------------------------

  if (L_Clipping)
    { // display text

      cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); // yellow

      cairo_move_to(cr, HOR_ORIGIN - 30, VER_ORIGIN + 3); // top

      cairo_show_text(cr, "Peak");
      cairo_stroke(cr);
    }

  if (R_Clipping)
    { // display text

      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); // red

      cairo_move_to(cr, HOR_ORIGIN - 30, VER_ORIGIN + LINE_WIDTH + LINE_SEP + 5); // top

      cairo_show_text(cr, "Peak");
      cairo_stroke(cr);
    }

  //---------------------
  //	level bars
  //---------------------

  double left_bar;

  left_bar = BAR_WIDTH * Lx; // bar length

  double right_bar;

  right_bar = BAR_WIDTH * Rx; // bar length

  cairo_set_source_rgb(cr, 1.0, 1.0, 0.3); // yellow

  cairo_move_to(cr, HOR_ORIGIN, VER_ORIGIN);
  cairo_line_to(cr, left_bar + HOR_ORIGIN, VER_ORIGIN);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 1.0, 0.0, 0.); // red

  cairo_move_to(cr, HOR_ORIGIN, VER_ORIGIN + LINE_WIDTH + LINE_SEP);
  cairo_line_to(cr, right_bar + HOR_ORIGIN, VER_ORIGIN + LINE_WIDTH + LINE_SEP);
  cairo_stroke(cr);

  //---------------------
  // 	microphone bar
  //---------------------

  static double mic_bar;
  mic_bar = BAR_WIDTH * ML;

  cairo_set_line_width(cr, LINE_WIDTH);

  if (ML < 95.0)
    {
      cairo_set_source_rgb(cr, 0.5, 1.0, 0.5); // green
    }
  else
    { // clipping

      cairo_set_source_rgb(cr, 0.5, 1.0, 0.5);                                            // green
      cairo_move_to(cr, HOR_ORIGIN - 25, VER_ORIGIN + 2 * LINE_WIDTH + 2 * LINE_SEP + 4); // top
      cairo_show_text(cr, "Peak");
      cairo_stroke(cr);

      cairo_set_source_rgb(cr, 0.9, 0.5, 1.0); // pink
    }

  MR = ML = 0.0;

  cairo_move_to(cr, HOR_ORIGIN, VER_ORIGIN + 2 * LINE_WIDTH + 2 * LINE_SEP);
  cairo_line_to(cr, mic_bar + HOR_ORIGIN, VER_ORIGIN + 2 * LINE_WIDTH + 2 * LINE_SEP);
  cairo_stroke(cr);

  return FALSE;
}

//--------------------------------------------------
//	frequency spectrum service function
//--------------------------------------------------

#define WindowWidth 200

int maxHeight = 100; // display window max from midpoint

void
fftTobars(fftw_complex* fft, int* bars)
{

  double strength, real, imagin, scale;
  int amplitude, i;

  scale = 0.0003;

  i = 0; // fft index

  for (size_t bar = 0; bar < WindowWidth / 2; bar++)
    { // average for each bar

      real = fft[i][0] * scale;
      imagin = fft[i][1] * scale;
      strength = real * real + imagin * imagin;
      i++;

      if (strength < 1.00e-10)
        {
          strength = 1.00e-10; // prevent overflows.
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
          bars[bar] = amplitude; // double up
        }
    }
}

//------------------------------------------------------
//	Calculate frequency spectrum array
//------------------------------------------------------

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
    { // used to perform Hann smoothing

      Hann = (float*)malloc(sizeof(float) * size);

      for (size_t n = 0; n < size; n++)
        {
          Hann[n] = 0.5 * (1.0 - cos(2.0 * M_PI * n / (size - 1.0)));
        }
    }

  //	fftw_plan plan = fftw_plan_dft_r2c_1d(size, in, out, FFTW_MEASURE); // alternates
  //	fftw_plan plan = fftw_plan_dft_r2c_1d(size, in, out, FFTW_PATIENT);

  fftw_plan plan = fftw_plan_dft_r2c_1d(size, in, out, FFTW_ESTIMATE);

  int barsL[WindowWidth / 2], barsR[WindowWidth / 2];

  //	------------
  //	left channel
  //	------------

  //	for (size_t i=0; i<size; i++) buffer[i] = exchangeBuf[i * 2]; // every other element

  for (size_t i = 0; i < size; i++)
    {
      in[i] = Hann[i] * exchangeBuf[i * 2]; // every other element
    }

  //	for(int i = 0; i < size; i++) in[i] = Hann[i] * buffer[i];

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
          //			else if (slchan[i] < 0.0) slchan[i] = 0.0;
        }
    }

  //	-------------
  //	right channel
  //	-------------

  //	for (size_t i=0; i<size; i++) buffer[i] = exchangeBuf[i * 2 + 1]; // every other element

  for (size_t i = 0; i < size; i++)
    {
      in[i] = Hann[i] * exchangeBuf[i * 2 + 1]; // every other element
    }

  //	for(int i = 0; i < size; i++) in[i] = Hann[i] * buffer[i];

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
          //			else if (srchan[i] < 0.0) srchan[i] = 0.0;
        }
    }

  fftw_free(in);
  fftw_free(out);

  exchange = 0;
}

void
frequency_marks(cairo_t* cr)
{

  // location of freq marks

#define SPECTRUM_FREQ_VERTICAL DRAW_HEIGHT - 10

  if (style == NARROW_SPECTRUM)
    { // narrow

      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // white

      cairo_move_to(cr, 1.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "kHz");
      cairo_stroke(cr);

      cairo_move_to(cr, 38.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "1");
      cairo_stroke(cr);

      cairo_move_to(cr, 88.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "2.5");
      cairo_stroke(cr);

      cairo_move_to(cr, 185.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "5");
      cairo_stroke(cr);

      cairo_move_to(cr, 270.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "7.5");
      cairo_stroke(cr);

      cairo_move_to(cr, 360.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "10");
      cairo_stroke(cr);

      return;
    }

  else if (style == WIDE_SPECTRUM)
    {
      cairo_set_source_rgb(cr, 1.2, 1.2, 1.0); // gray

      cairo_move_to(cr, 1.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "kHz");
      cairo_stroke(cr);

      cairo_move_to(cr, 40.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "2.5");
      cairo_stroke(cr);

      cairo_move_to(cr, 88.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "5");
      cairo_stroke(cr);

      cairo_move_to(cr, 130.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "7.5");
      cairo_stroke(cr);

      cairo_move_to(cr, 176.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "10");
      cairo_stroke(cr);

      cairo_move_to(cr, 270.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "15");
      cairo_stroke(cr);

      cairo_move_to(cr, 360.0, SPECTRUM_FREQ_VERTICAL); // top
      cairo_show_text(cr, "20");
      cairo_stroke(cr);
    }
}
