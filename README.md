# FM Radio for the Xperia Z2 on Phosh

A GTK4/libadwaita FM radio app for the Sony Xperia Z2, and the small service it
needs to reach the tuner.

No existing Linux FM app works on this phone: they all expect a kernel radio
device (`/dev/radio0`), and the Z2's tuner is the FM core inside the Broadcom
BCM4335C0 Bluetooth chip, driven by HCI vendor commands. See
the hardware notes in the
port's own repository, `RobLymm/xperia-sirius-linux`.

## Parts

`sirius-fmd`
: a root service that owns a raw HCI socket and speaks the Broadcom FM
  protocol directly (no `hcitool` subprocesses). It offers a JSON-lines
  protocol on `/run/sirius-fm/sirius-fm.sock`, readable by the `audio` group:
  power, tune, status, RDS station name, audio output, and a streaming band
  scan that finds signal peaks and names them from RDS. A tune, power-off or
  cancel request stops a running scan, and status answers during one.

`sirius-fm`
: the app. Frequency, station name, signal meter, previous/next station,
  fine tuning and play/stop. Two buttons at the top right:

  - **Output.** Its icon shows the output in use (speaker, Bluetooth or
    headphones) and it opens the list to change it. Bluetooth is greyed when
    no audio device is connected and lists each connected device otherwise.
    Headphones are greyed when the WCD9320 codec is not loaded. The last
    choice is remembered and used whenever it is available; otherwise the
    speaker.

    The speaker and the headphone jack are the same PCM with different
    routing, so choosing between them means setting mixer controls rather
    than choosing a sink. The sound server will not do it: PulseAudio turns
    two routes that share a PCM into two card profiles rather than two ports
    on one sink, and switching profile drops the sink and often fails to
    bring the new one up. So the app sets the route itself, when playback
    starts, and puts it back to the speaker when playback stops. Nothing
    detects whether anything is plugged into the jack; the codec's jack
    detection is not driven yet.
  - **Stations.** The saved station list. A scan runs in the background each
    time the app opens (about 40 seconds). The saved list stays usable
    meanwhile and is only replaced when the scan finds stations, so opening
    the app without the headphone lead plugged in keeps it. Choosing a
    station or pressing play stops the scan.

## Audio

Headphones must be plugged in whichever output is chosen: the lead is the
aerial.

Tuning, scanning, station names and playback work. The service switches the
tuner chip's PCM pads to FM I2S with the chip as clock master, and the app
captures the stream from the sound card's secondary MI2S port (front end
`MultiMedia2`, found by name) and plays it to the chosen output with `pacat`.
The capture is read through the `sirius_fm` ALSA device when the Xperia Z2
port's `fmrepair` plugin is installed (`drivers/audio/fmrepair` in that
project): the tuner's I2S link corrupts the sign bit of a burst of samples
41.6 times a second, and the plugin repairs those bursts at the device layer
so this app, and any other client, gets clean audio. Without the plugin the
app reads the raw device and the buzz is audible. The Sound menu's Mono and
Noise reduction modes (`sirius-fm-downmix`, a small C helper built by
install.sh: mono sum, optional 12 kHz low-pass) are for weak-signal stereo
hiss, the way a hardware radio blends to mono; they are optional. The
sound-card side (device tree links, machine driver, fmrepair plugin) is in
the Xperia Z2 project; see the hardware notes in the
port's own repository, `RobLymm/xperia-sirius-linux`.

## Install

    ./install.sh

## Status

Tested on the phone 2026-09-13: the service, scan (14 stations found and
named in 42 s), scan cancellation, the output list, and BBC Radio 1
through the speaker. 2026-09-14: the background "flicking" was traced to
sign-bit corruption in the I2S capture and is repaired by the port's
fmrepair ALSA plugin (0–1 glitches/s live, against ~900 raw); the app now
reads `sirius_fm` when the plugin is present.
