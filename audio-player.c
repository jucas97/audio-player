
/*
 * Copyright (C) 2022 Joel Pinto <joelstubemail@gmail.com>
 */
#include <stdio.h>

#include <gst/gst.h>

typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2)
} GstPlayFlags;

typedef struct _CustomData
{ 
  GstElement *playbin;
  GMainLoop *loop;
  gboolean playing;             /* Playing or Paused */
} CustomData;

static void eos_callback (GstBus *, GstMessage *, CustomData *);
static void error_callback (GstBus *, GstMessage *, CustomData *);
static gboolean handle_keyboard (GIOChannel *, GIOCondition, CustomData *);

int main (int argc, char *argv[])
{
  CustomData data;
  GstStateChangeReturn ret;
  GstBus *bus;
  GIOChannel *io_stdin;
  gint flags;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Initialize our data structure */
  memset (&data, 0, sizeof (data));

  /* Build the pipeline */
  data.playbin = gst_element_factory_make ("playbin", "playbin");

  g_object_set (data.playbin, "uri", "file:///home/joel/Downloads/exits-official-music-video.mp3", NULL);

  /* Set flags to show Audio and Video but ignore Subtitles */
  g_object_get (data.playbin, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_AUDIO;
  flags &= ~GST_PLAY_FLAG_VIDEO & ~GST_PLAY_FLAG_TEXT;
  g_object_set (data.playbin, "flags", flags, NULL);

  bus = gst_element_get_bus (data.playbin);
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message::error", (GCallback) error_callback, &data);
  g_signal_connect (G_OBJECT (bus), "message::eos", (GCallback) eos_callback, &data);
  gst_object_unref(bus);

  io_stdin = g_io_channel_unix_new (fileno (stdin));
  g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc) handle_keyboard, &data);

  ret = gst_element_set_state (data.playbin, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.playbin);
    return -1;
  }

  data.playing = TRUE;

  /* Create a GLib Main Loop and set it to run */
  data.loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (data.loop);

  /* Free resources */
  g_main_loop_unref (data.loop);
  gst_element_set_state (data.playbin, GST_STATE_NULL);
  gst_object_unref (data.playbin);
  return 0;
}

static void eos_callback (GstBus *bus, GstMessage *msg, CustomData *data) {
  g_print("EOS\n");
}

static void error_callback (GstBus *bus, GstMessage *msg, CustomData *data) {
  GstStateChangeReturn ret;
  GError *error;

  gst_message_parse_error(msg, &error, NULL);
  g_printerr("Error from component %s, error %s\n", GST_OBJECT_NAME(msg->src), error->message);
  g_error_free(error);

  ret = gst_element_set_state (data->playbin, GST_STATE_READY);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the ready state.\n");
    gst_object_unref (data->playbin);
  }
}

/* Process keyboard input */
static gboolean handle_keyboard (GIOChannel * source, GIOCondition cond, CustomData * data)
{
  gchar *str = NULL;
  gboolean g_mute = FALSE;

  if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL) {
    return TRUE;
  }

  switch (g_ascii_tolower (str[0])) {
    case 'p':
      data->playing = !data->playing;
      gst_element_set_state (data->playbin, data->playing ? GST_STATE_PLAYING : GST_STATE_PAUSED);
      g_print ("Setting state to %s\n", data->playing ? "PLAYING" : "PAUSE");
      break;
    case 'q':
      g_main_loop_quit (data->loop);
      break;
    case 'm':
      g_object_get(data->playbin, "mute", &g_mute, NULL);
      g_object_set(data->playbin, "mute", !g_mute, NULL);
      break;
    default:
      break;
  }

  g_free (str);

  return TRUE;
}
