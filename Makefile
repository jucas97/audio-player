CC = gcc
CFLAGS = `pkg-config --cflags --libs gstreamer-video-1.0 gtk+-3.0 gstreamer-1.0`

audio-player: audio-player.c
	${CC} audio-player.c -o audio-player ${CFLAGS}

clean:
	rm -f audio-player
