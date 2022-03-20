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

// Jan 8, 2022

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

char*
sink_name()
{
  static char x[32];
  sprintf(x, "sink%d", getpid());
  return x;
}
char*
source_name()
{
  static char x[32];
  sprintf(x, "source%d", getpid());
  return x;
}

static const pa_sample_spec sampSpec = {
  .format = PA_SAMPLE_FLOAT32LE,
  .rate = 44100,
  .channels = 2};

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

//----------------------------------------
//	internal channel callbacks
//----------------------------------------

//-----------------------------
//	Player callback
//-----------------------------

void
stream_read_callback_sink(pa_stream* s, size_t l, void* dmy)
{

  const void* p;
  static int count = 0;

  if (pa_stream_peek(s, &p, &l) < 0)
    {
      g_message("pa_stream_peek() failed: %s", pa_strerror(pa_context_errno(context_sink)));
      return;
    }

  float* audio_data = (float*)p;
  int samples = l / sizeof(float);
  int nchan = sink_channels;
  float levels[2] = {0.0};

  if (samples < 2)
    {
      pa_stream_drop(s);
      return;
    }

  if (samples > SAMPLE_SIZE)
    { // for spectrum analysis
      for (size_t i = 0; i < SAMPLE_SIZE; i++)
        {
          exchangeBuf[i] = audio_data[i];
        }
      exchange = 1;
    }

  while (samples >= nchan)
    {
      for (size_t c = 0; c < nchan; c++)
        {
          float v = fabs(audio_data[c]);
          if (v > levels[c])
            {
              levels[c] = v;
            }
        }
      audio_data += nchan;
      samples -= nchan;
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

//--------------------------------------
//      Microphone channel callback
//--------------------------------------

void
stream_read_callback_source(pa_stream* s, size_t len, void* user)
{

  const void* p;
  static int count = 0;
  static float inData[5];

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
  int samples = len / sizeof(float);
  int nchan = source_channels;
  float levels[2] = {0.0};

  if (samples < 2)
    {
      pa_stream_drop(s);
      return;
    }

  while (samples >= nchan)
    {
      for (size_t c = 0; c < nchan; c++)
        {
          float v = fabs(pcm[c]);
          if (v > levels[c])
            {
              levels[c] = v;
            }
        }
      pcm += nchan;
      samples -= nchan;
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

  if (nchan == 2)
    {
      ML = (ML + MR) / 2; // average
    }
  else
    {
      MR = ML; // mono
    }

  ML = log10(ML + 1) * (FACTOR - 20.0);

  pa_stream_drop(s);
}

//------------------------------------------------
//	stream state callback to receive messages
//------------------------------------------------

//-----------------
//	Sink
//-----------------

void
stream_state_callback_sink(pa_stream* s, void* dmy)
{

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

//---------------
//	Source
//---------------

void
stream_state_callback_source(pa_stream* s, void* dmy)
{

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

//---------------------------------------------
//	stream create - used for all channels
//---------------------------------------------

void
sink_create_stream(const char* name, const char* description,
                   const pa_sample_spec sampSpec, const pa_channel_map cmap)
{

  // PLAY

  char txt[256];
  pa_sample_spec nss;

  /*------------------------------------------------------------------------------------
  pa_buffer_attr
    maxlength = -1;	// Maximum length of the buffer in bytes.
    tlength = -1;		// Playback only: target length of the buffer.
    prebuf = -1;		// Playback only: pre-buffering.
    minreq = -1;		// Playback only: minimum request.
    fragsize = -1;	// Recording only: server sends data in fragsize block
  ---------------------------------------------------------------------------------------*/

  //-------------------------------------
  //	stream device (channel) name
  //-------------------------------------

  g_free(device_name_sink);
  device_name_sink = g_strdup(device_name_sink);

  g_free(device_description_sink);
  device_description_sink = g_strdup(device_description_sink);

  nss.format = PA_SAMPLE_FLOAT32;
  nss.rate = sampSpec.rate;
  nss.channels = sampSpec.channels; // check your mic - mono or stereo?

  printf("\nPlayback Sample format for device %s:\n\t%s\n",
         device_name_sink, pa_sample_spec_snprint(txt, sizeof(txt), &nss));

  printf("Playback Create stream Channel map for device %s:\n\t%s\n",
         device_name_sink, pa_channel_map_snprint(txt, sizeof(txt), &cmap));

  //-------------------------------------------------------------------------
  //	establish callbacks and connection for internal players (sinks)
  //-------------------------------------------------------------------------

  stream_sink = pa_stream_new(context_sink, PULSE_SINK_NAME, &nss, &cmap);

  //------------------------------------------------------------------
  //	establish function to be called for status report
  //------------------------------------------------------------------

  pa_stream_set_state_callback(stream_sink, stream_state_callback_sink, NULL);

  //------------------------------------------------------------------------
  //	establish function to be called when data is available
  //------------------------------------------------------------------------

  pa_stream_set_read_callback(stream_sink, stream_read_callback_sink, NULL); // establish callback fcn

  //-----------------------------------------------------------------------------------------
  //	alternate initializations
  //	pa_stream_connect_record(stream, name, NULL, (enum pa_stream_flags) 0);
  //	pa_stream_connect_record(stream, name, &paba, (enum pa_stream_flags) 0);
  //------------------------------------------------------------------------

  //--------------------------------------
  //	connect to stream
  //--------------------------------------

  //	pa_buffer_attr  paba_sink = { 4000, -1, -1, -1, -1 } ;

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

  // MIKE

  char txt[256];
  pa_sample_spec nss;

  /*------------------------------------------------------------------------------------
  pa_buffer_attr
    maxlength = -1;	// Maximum length of the buffer in bytes.
    tlength = -1;		// Playback only: target length of the buffer.
    prebuf = -1;		// Playback only: pre-buffering.
    minreq = -1;		// Playback only: minimum request.
    fragsize = -1;	// Recording only: server sends data in fragsize block
  ---------------------------------------------------------------------------------------*/

  //-------------------------------------
  //	stream device (channel) name
  //-------------------------------------

  g_free(device_name_source);
  device_name_source = g_strdup(device_name_source);

  g_free(device_description_source);
  device_description_source = g_strdup(device_description_source);

  nss.format = PA_SAMPLE_FLOAT32;
  nss.rate = sampSpec.rate;
  nss.channels = sampSpec.channels; // check your mic - mono or stereo?

  printf("\n+++ source Sample format for device %s\n",
         pa_sample_spec_snprint(txt, sizeof(txt), &nss));

  printf("+++ source Create stream Channel map %s\n",
         pa_channel_map_snprint(txt, sizeof(txt), &cmap));

  //-------------------------------------------------------------------------
  //	establish callbacks and connection for internal players (sinks)
  //-------------------------------------------------------------------------

  stream_source = pa_stream_new(context_source, PULSE_SOURCE_NAME, &nss, &cmap);

  //------------------------------------------------------------------
  //	establish function to be called for status report
  //------------------------------------------------------------------

  pa_stream_set_state_callback(stream_source, stream_state_callback_source, NULL);

  //------------------------------------------------------------------------
  //	establish function to be called when data is available
  //------------------------------------------------------------------------

  pa_stream_set_read_callback(stream_source, stream_read_callback_source, NULL);

  //--------------------------------------
  //	connect to stream
  //--------------------------------------

  pa_buffer_attr paba_sink = {4000, -1, -1, -1, 4000}; // adjust sample size

  pa_stream_connect_record(stream_source, name, &paba_sink, (enum pa_stream_flags)PA_STREAM_ADJUST_LATENCY);
}

//--------------------------------------------------
//	source (microphone) information callback
//--------------------------------------------------

void
source_info_callback(pa_context* p, const pa_source_info* si, int is_last, void* dmy)
{

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

  char txt[255];

  source_channels = si->channel_map.channels;

  printf("++ channel map for device: %s\n",
         pa_channel_map_snprint(txt, sizeof(txt), &si->channel_map));

  printf("+++ name: %s\n", si->name);
  printf("+++ index: %d\n", si->index);
  printf("+++ volume: %d\n", si->volume);
  printf("+++ n_ports: %d\n", si->n_ports);
  printf("\n");

  if (si->n_ports > 0)
    {
      for (size_t i = 0; i < si->n_ports; i++)
        {
          printf("+++ pa_source_port_info name %d: %s\n", i, si->ports[i]->name);
          printf("+++ pa_source_port_info priority %d: %d\n", i, si->ports[i]->priority);
          printf("+++ pa_source_port_info avail %d: %d\n", i, si->ports[i]->available == PA_PORT_AVAILABLE_YES);
        }
      printf("+++ pa_source_port_info active port name: %s\n", si->active_port->name);
      printf("\n");
      printf("+++ source channels %d\n", source_channels);
      printf("+++ map to name: %s\n", pa_channel_map_to_name(&si->channel_map));
      printf("+++ description: %s\n", si->description);
    }

  source_create_stream(si->name, si->description, si->sample_spec, si->channel_map);
}

//--------------------------------------------------
//	information callback
//--------------------------------------------------

void
sink_info_callback(pa_context* p, const pa_sink_info* si, int is_last, void* dmy)
{

  // PLAYBACK

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

  printf("sink channels %d\n", sink_channels);

  sink_create_stream(si->monitor_source_name, si->description, si->sample_spec, si->channel_map);
}

void
sink_server_info_callback(pa_context* c, const pa_server_info* si, void* dmy)
{

  // PLAYBACK

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
    NULL);

  pa_operation_unref(p1);
}

void
source_server_info_callback(pa_context* c, const pa_server_info* si, void* dmy)
{

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

  //----------------------------------------------------------------------------------------
  //		if there is an external source (microphone), its name begins 'alsa_input'
  //----------------------------------------------------------------------------------------

  printf("default source name =  %s\n", si->default_source_name);
  if (strncmp(si->default_source_name, "alsa_input", 10) == 0 && strncmp(si->default_source_name, "RDPSource", 9) == 0)
    {
      printf("*** no microphone detected\n");
      no_microphone = 1;
      return;
    }

  printf("*** microphone found\n");
  no_microphone = 0;

  //                if (strncmp(si->default_source_name, "alsa_input", 10) != 0) {
  //                        printf("*** no microphone detected\n");
  //                        no_microphone = 1;
  //                        return;
  //                        }

  //                printf("*** microphone found\n");
  //                no_microphone = 0;

  //----------------------------------------------------------
  //		establish call back to receive source info
  //----------------------------------------------------------

  printf("default source name %s\n", si->default_source_name);
  p1 = pa_context_get_source_info_by_name(c, si->default_source_name, source_info_callback, NULL);

  pa_operation_unref(p1);
}

void
source_connection_state_callback(pa_context* c, void* dmy)
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

      p1 = pa_context_get_server_info(c, source_server_info_callback, dmy);
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

    default:
      printf("+++++ default\n");
    }
}

void
sink_connection_state_callback(pa_context* c, void* dmy)
{

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
        NULL);

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

    default:
      printf("+++++ default\n");
    }
}
