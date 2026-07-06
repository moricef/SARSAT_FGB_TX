#ifndef HOMING_GENERATOR_H
#define HOMING_GENERATOR_H

#include <stddef.h>
#include "biphase_modulator.h"   /* iq_sample_t */

// ELT swept-tone AM homing signal: AM carrier, audio tone swept downward.
// Sweep 1600 -> 300 Hz, 3 sweeps/s, modulation index 0.90.

#define HOMING_SWEEP_F_START_HZ 1600.0
#define HOMING_SWEEP_F_END_HZ    300.0
#define HOMING_SWEEP_RATE_HZ       3    /* sweeps per second; must divide fs */
#define HOMING_MOD_INDEX         0.90

// Generate exactly one sweep period at sample_rate (replayed back-to-back).
// Returns malloc'd buffer (caller frees) and its length, or NULL on error.
iq_sample_t* homing_generate_sweep(unsigned sample_rate, int amplitude,
                                   size_t *num_samples);

#endif // HOMING_GENERATOR_H
