#include "biphase_modulator.h"
#include "bessel_filter.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Amplitude for 16-bit samples (max = 32767)
// Reduced: 8000 (~-6dB), 4000 (~-12dB), 2000 (~-18dB), 1600 (~-20dB), 1000 (~-24dB)
#define IQ_AMPLITUDE 1600

// BPSK phase shift (T.001 compatible: ±1.1 radians, not 0/π)
#define PHASE_SHIFT_RAD 1.1  // ±1.1 rad phase deviation

// =============================
// Biphase-L (Manchester) Encoding
// =============================

void generate_biphase_baseband(const uint8_t *frame_bits, int num_bits,
                                int16_t *baseband, size_t *num_samples) {
    size_t sample_idx = 0;

    for (int bit_idx = 0; bit_idx < num_bits; bit_idx++) {
        uint8_t bit = frame_bits[bit_idx];

        // Biphase-L encoding (Manchester):
        // bit 0: low→high transition (0 first half, 1 second half)
        // bit 1: high→low transition (1 first half, 0 second half)

        for (int sample = 0; sample < SAMPLES_PER_BIT; sample++) {
            if (bit == 0) {
                // 0→1 transition at mid-bit
                baseband[sample_idx++] = (sample < SAMPLES_PER_BIT/2) ? 0 : 1;
            } else {
                // 1→0 transition at mid-bit
                baseband[sample_idx++] = (sample < SAMPLES_PER_BIT/2) ? 1 : 0;
            }
        }
    }

    *num_samples = sample_idx;
}

// =============================
// BPSK I/Q Modulation
// =============================

void modulate_bpsk_iq(const int16_t *baseband, size_t num_baseband_samples,
                      iq_sample_t *iq_samples, size_t *num_iq_samples) {
    size_t iq_idx = 0;

    for (size_t i = 0; i < num_baseband_samples; i++) {
        // BPSK with ±1.1 rad phase shift (T.001 spec)
        // baseband=0 -> phase=-1.1 rad
        // baseband=1 -> phase=+1.1 rad

        double phase = (baseband[i] == 0) ? -PHASE_SHIFT_RAD : PHASE_SHIFT_RAD;

        // I/Q generation: I = A*cos(phase), Q = A*sin(phase)
        int16_t i_val = (int16_t)(IQ_AMPLITUDE * cos(phase));
        int16_t q_val = (int16_t)(IQ_AMPLITUDE * sin(phase));

        // Simple interpolation: repeat each sample INTERPOLATION_FACTOR times.
        // PLUTO_SAMPLE_RATE is chosen so this is an integer and preserves baud.
        for (int interp = 0; interp < INTERPOLATION_FACTOR; interp++) {
            iq_samples[iq_idx].i = i_val;
            iq_samples[iq_idx].q = q_val;
            iq_idx++;
        }
    }

    *num_iq_samples = iq_idx;
}

// =============================
// Complete T.001 Waveform
// =============================

void generate_t001_waveform(const uint8_t *frame_bits,
                             iq_sample_t **waveform,
                             size_t *waveform_length) {
    // Calculate lengths
    size_t carrier_samples = (CARRIER_DURATION_MS * PLUTO_SAMPLE_RATE) / 1000;
    size_t message_samples = (MESSAGE_DURATION_MS * PLUTO_SAMPLE_RATE) / 1000;
    size_t total_samples = carrier_samples + message_samples;

    // Allocate waveform
    *waveform = malloc(total_samples * sizeof(iq_sample_t));
    if (!*waveform) {
        *waveform_length = 0;
        return;
    }

    size_t wf_idx = 0;

    // 1. Unmodulated carrier (160 ms): I=A, Q=0
    for (size_t i = 0; i < carrier_samples; i++) {
        (*waveform)[wf_idx].i = IQ_AMPLITUDE;
        (*waveform)[wf_idx].q = 0;
        wf_idx++;
    }

    // 2. Generate Biphase-L baseband (6400 Hz)
    size_t baseband_samples = 144 * SAMPLES_PER_BIT;  // 144 bits * 16 samples/bit
    int16_t *baseband = malloc(baseband_samples * sizeof(int16_t));
    if (!baseband) {
        free(*waveform);
        *waveform = NULL;
        *waveform_length = 0;
        return;
    }

    size_t actual_baseband_samples = 0;
    generate_biphase_baseband(frame_bits, 144, baseband, &actual_baseband_samples);

    // 3. Modulate to I/Q (interpolate to PLUTO_SAMPLE_RATE)
    size_t iq_data_samples = 0;
    iq_sample_t *iq_data = malloc(actual_baseband_samples * INTERPOLATION_FACTOR * sizeof(iq_sample_t));
    if (!iq_data) {
        free(baseband);
        free(*waveform);
        *waveform = NULL;
        *waveform_length = 0;
        return;
    }

    modulate_bpsk_iq(baseband, actual_baseband_samples, iq_data, &iq_data_samples);

    // 4. Append raw modulated data to waveform (after carrier)
    for (size_t i = 0; i < iq_data_samples && wf_idx < total_samples; i++) {
        (*waveform)[wf_idx++] = iq_data[i];
    }

    // 5. Bessel filter over the whole burst (carrier + data), in place.
    // Filtering data alone from zero state caused an amplitude transient
    // at message start. bessel_process is in-place safe.
    bessel_state_t bessel_state;
    bessel_init(&bessel_state);
    bessel_process(&bessel_state, *waveform, *waveform, wf_idx);

    // Cleanup
    free(baseband);
    free(iq_data);

    *waveform_length = wf_idx;
}

void free_t001_waveform(iq_sample_t *waveform) {
    free(waveform);
}
