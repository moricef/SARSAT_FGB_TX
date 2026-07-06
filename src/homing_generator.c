#include "homing_generator.h"
#include <stdlib.h>
#include <math.h>

// AM on I, Q = 0. Envelope (1 + m*sin(phi)) / (1 + m) so peak = amplitude.
// Tone frequency swept linearly downward over one period; segments are
// replayed back-to-back (sawtooth sweep, like a real ELT).
iq_sample_t* homing_generate_sweep(unsigned sample_rate, int amplitude,
                                   size_t *num_samples) {
    if (sample_rate % HOMING_SWEEP_RATE_HZ != 0) {
        *num_samples = 0;
        return NULL;   /* period must be an integer number of samples */
    }

    size_t n = sample_rate / HOMING_SWEEP_RATE_HZ;
    iq_sample_t *buf = malloc(n * sizeof(iq_sample_t));
    if (!buf) {
        *num_samples = 0;
        return NULL;
    }

    const double m = HOMING_MOD_INDEX;
    const double a = (double)amplitude / (1.0 + m);
    double phi = 0.0;

    for (size_t i = 0; i < n; i++) {
        double t = (double)i / (double)n;
        double f = HOMING_SWEEP_F_START_HZ
                 + t * (HOMING_SWEEP_F_END_HZ - HOMING_SWEEP_F_START_HZ);
        phi += 2.0 * M_PI * f / (double)sample_rate;
        buf[i].i = (int16_t)(a * (1.0 + m * sin(phi)));
        buf[i].q = 0;
    }

    *num_samples = n;
    return buf;
}
