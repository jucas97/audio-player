
/*
 * Copyright (C) 2022 Joel Pinto <joelstubemail@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>

#include <gst/gst.h>

#define MEDIAFILE_LENGTH 128

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
  gchar *playlist_path;
  gchar *media_file;
  gint next_playlist_index;
} CustomData;

static void eos_callback (GstBus *, GstMessage *, CustomData *);
static void error_callback (GstBus *, GstMessage *, CustomData *);
static gboolean handle_keyboard (GIOChannel *, GIOCondition, CustomData *);
static gboolean select_media_file(FILE *, CustomData *);
static void set_playbin_uri(CustomData *);

int main (int argc, char *argv[])
{
  CustomData data;
  GstStateChangeReturn ret;
  GstBus *bus;
  GIOChannel *io_stdin;
  gint flags;
 gint return_code;

  if (argc == 1) {
    g_printerr("Missing playlist path");
    return_code = -1;
    goto error_media_file_allocation;
  }

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Initialize our data structure */
  memset (&data, 0, sizeof (data));

  /* Playlist file */
  data.playlist_path = argv[1];

  /* Memory allocation for the mediafile */
  data.media_file = (gchar *) calloc(MEDIAFILE_LENGTH, sizeof(gchar));
  if (data.media_file == NULL) {
    g_printerr("Failed to allocate memory for mediafile");
    return_code = -1;
    goto error_media_file_allocation;
  }

  /* Build the pipeline */
  data.playbin = gst_element_factory_make ("playbin", "playbin");

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

  set_playbin_uri(&data);
  if (!data.playing) {
    g_printerr("Failed to set playbin uri\n");
    goto error_set_playbin_uri;
  }

  /* Create a GLib Main Loop and set it to run */
  data.loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (data.loop);

  /* Free resources */
  g_main_loop_unref (data.loop);
error_set_playbin_uri:
  gst_element_set_state (data.playbin, GST_STATE_NULL);
  gst_object_unref (data.playbin);
  free(data.media_file);
error_media_file_allocation:
  return return_code;
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

static gboolean select_media_file(FILE *file, CustomData *data)
{
  gchar *next_media_file = NULL;
  gchar c;
  gint next_media_file_index=0;
  gint playlist_index = 0;
  gboolean found_next = FALSE;

  if (file == NULL) {
    g_printerr("file ptr null\n");
    return found_next;
  }

  next_media_file = (gchar *) calloc(MEDIAFILE_LENGTH, sizeof(gchar));
  if (next_media_file == NULL) {
    g_printerr("Failed to allocate memory\n");
    return found_next;
  }

  c = (gchar) fgetc(file);
  while (c != EOF) {
    if (c == '\n') {
      if (data->next_playlist_index == playlist_index) {
        ++next_media_file_index;
        *(next_media_file + next_media_file_index) = '\0';
        memset(data->media_file, 0, MEDIAFILE_LENGTH);
        memcpy(data->media_file, next_media_file, next_media_file_index);
        found_next = TRUE;
        break;
      }

      memset(next_media_file, 0, next_media_file_index);
      next_media_file_index = 0;
      ++playlist_index;
      c = (gchar) fgetc(file);
    }

    if (next_media_file_index == MEDIAFILE_LENGTH) {
      g_printerr("Reached maximum path size");
      break;
    }

    *(next_media_file + next_media_file_index) = c;
    ++next_media_file_index;
    c = (gchar) fgetc(file);
  }

  free(next_media_file);
  return found_next;
}

static void set_playbin_uri(CustomData *data)
{
  FILE *playlist = NULL;
  GstStateChangeReturn ret;
  GstState state;

  /* Reading playlist */
  playlist = fopen(data->playlist_path, "r");
  if (playlist == NULL) {
    g_printerr("Failure opening file %s\n", data->playlist_path);
    return;
  }

  ret = gst_element_get_state(data->playbin, &state, NULL, GST_CLOCK_TIME_NONE);
  if (ret != GST_STATE_CHANGE_SUCCESS) {
    g_printerr("Unable to fetch latest state");
    goto close;
  }

  if (state == GST_STATE_PLAYING) {
    ret = gst_element_set_state(data->playbin, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      g_printerr ("Unable to set the pipeline to the ready state.\n");
      goto close;
    }
  }

  if (select_media_file(playlist, data)) {
    g_object_set(data->playbin, "uri", data->media_file, NULL);

    ret = gst_element_set_state (data->playbin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      g_printerr ("Unable to set the pipeline to the playing state.\n");
      goto close;
    }

    data->playing = TRUE;
  }

close:
  fclose(playlist);
}
