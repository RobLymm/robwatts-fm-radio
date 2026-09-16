// SPDX-License-Identifier: GPL-2.0-only
// Mono sum and optional low-pass for the Broadcom FM capture from the Xperia
// Z2's secondary MI2S port.
//
// Stages: (1) optional L+R mono sum (FM stereo on a weak signal carries the
// L-R difference as noise; the sum drops it); (2) optional 12 kHz low-pass.
//
//   sirius-fm-downmix               mono + low-pass ("nr")
//   sirius-fm-downmix --no-lowpass  mono ("mono")
//   sirius-fm-downmix --stereo      pass-through, stereo out ("stereo")
//   sirius-fm-downmix --cutoff 9000 low-pass corner
//
// The tuner's periodic I2S burst corruption (the "flicking") is repaired
// below this, at the ALSA device layer, by the port's fmrepair plugin
// (drivers/audio/fmrepair); this helper only sums and filters.
//
// C, not shell/python: the pure-Python version cannot sustain 48 kHz here.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FS 48000.0
#define FC 12000.0

struct biquad { double b0, b1, b2, a1, a2, x1, x2, y1, y2; };

static void design(struct biquad *bq, double fc, double fs, double q)
{
	double w0 = 2.0 * M_PI * fc / fs;
	double c = cos(w0), s = sin(w0), alpha = s / (2.0 * q), a0 = 1.0 + alpha;
	bq->b0 = (1.0 - c) / 2.0 / a0;
	bq->b1 = (1.0 - c) / a0;
	bq->b2 = (1.0 - c) / 2.0 / a0;
	bq->a1 = -2.0 * c / a0;
	bq->a2 = (1.0 - alpha) / a0;
	bq->x1 = bq->x2 = bq->y1 = bq->y2 = 0.0;
}

static double run(struct biquad *bq, double x)
{
	double y = bq->b0 * x + bq->b1 * bq->x1 + bq->b2 * bq->x2
		 - bq->a1 * bq->y1 - bq->a2 * bq->y2;
	bq->x2 = bq->x1; bq->x1 = x;
	bq->y2 = bq->y1; bq->y1 = y;
	return y;
}

static int16_t clip(long v)
{
	return v < -32768 ? -32768 : v > 32767 ? 32767 : (int16_t)v;
}

int main(int argc, char **argv)
{
	int lowpass = 1, mono = 1;
	double fc = FC;
	for (int a = 1; a < argc; a++) {
		if (!strcmp(argv[a], "--no-lowpass")) lowpass = 0;
		else if (!strcmp(argv[a], "--stereo")) mono = lowpass = 0;
		else if (!strcmp(argv[a], "--cutoff") && a + 1 < argc) fc = atof(argv[++a]);
	}

	struct biquad s1, s2;
	design(&s1, fc, FS, 0.54119610);
	design(&s2, fc, FS, 1.30656296);

	int16_t in[4096];
	int16_t out[4096];
	size_t n;

	while ((n = fread(in, sizeof(int16_t), 4096, stdin)) >= 2) {
		size_t frames = n / 2, i, w = 0;
		for (i = 0; i < frames; i++) {
			int16_t rL = in[2 * i], rR = in[2 * i + 1];
			if (mono) {
				double v = ((double)rL + rR) * 0.5;
				if (lowpass) v = run(&s2, run(&s1, v));
				out[w++] = clip(lround(v));
			} else {
				out[w++] = rL; out[w++] = rR;
			}
		}
		if (w && fwrite(out, sizeof(int16_t), w, stdout) < w) break;
		fflush(stdout);
	}
	return 0;
}
