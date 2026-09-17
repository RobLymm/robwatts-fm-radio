// SPDX-License-Identifier: GPL-2.0-only
// Mono sum for the Broadcom FM capture from the Xperia Z2's secondary MI2S
// port.
//
// FM stereo carries the L-R difference, and on a weak signal that difference
// is mostly hiss, so summing the two channels drops it. That is all this
// does; it is what a hardware radio does when it blends to mono.
//
//   sirius-fm-downmix    stereo in on stdin, mono out on stdout
//
// The tuner's periodic I2S burst corruption (the "flicking") is repaired
// below this, at the ALSA device layer, by the port's fmrepair plugin
// (drivers/audio/fmrepair); this helper only sums.
//
// C, not shell/python: the pure-Python version cannot sustain 48 kHz here.
#include <stdint.h>
#include <stdio.h>

static int16_t clip(long v)
{
	return v < -32768 ? -32768 : v > 32767 ? 32767 : (int16_t)v;
}

int main(void)
{
	int16_t in[4096];
	int16_t out[2048];
	size_t n;

	while ((n = fread(in, sizeof(int16_t), 4096, stdin)) >= 2) {
		size_t frames = n / 2, i;

		for (i = 0; i < frames; i++)
			out[i] = clip(((long)in[2 * i] + in[2 * i + 1]) / 2);

		if (frames && fwrite(out, sizeof(int16_t), frames, stdout) < frames)
			break;
		fflush(stdout);
	}
	return 0;
}
