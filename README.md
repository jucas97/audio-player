# Custom command line based Audio-Player

## Overview

This repository contains a custom command line based audio player. The player constructs a Gstreamer pipeline through a stand-alone everything-in-one abstraction (playbin gst element) to play audio content.
Tha audio files to be played are delivered to the application as plaintext by the parse of a playlist file. Each line of the playlist path representes an entry for an audio file i.e., the source type and its location in the local disk (currently only the case for audio files persisted in the disk is ensured to properly work).

The user can interact with the player to perform following actions: (i) pause and/or resume the stream; (ii) mute and unmute; (iii) forward to the next audio file; (iv) go backwards to the previous audio file; (v) close the player.

## Setup

As mentioned in the *Overview* section, the playlist file is composed by the specification of the location and source type of each media file. An example is displayed below:

```
file:///home/yyy/Downloads/ainda-nao-acabei.mp3
file:///home/yyy/Downloads/tttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttestmediafilelen.mp3
file:///home/yyy/Downloads/neptune-official-music-video.mp3
```

To build the audio player, just compile the audio-player target.
``` make audio-player ```

Execute the binary with the playlist file location passed as argument.
``` ./audio-player ~/playlist.txt ```

To compile with debug symbols, just add the -g flag to the audio-player rule.
One way to debug the player, is to execute it with GNU Debugger.
``` gdb --args audio-player ~/playlist.txt ```

## TODO

* Implement a message popup notification mechanism to notify the user of which audio file is being played (instead of dumping it to the command line). WCS -> dbus com to ad hoc event based daemon to spawn and execute notify-send;
* Volume control. Gst Playbin element already encompasses the volume property, just required to change it with *g_object_set()*;
* Close application when playlist file reaches the end -> Quit Glib main loop and free resources;
* Confirm there is no memory leak and dangling pointers (gdb, valgrind ...);
* Specify user interface.
