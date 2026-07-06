#include "bessel_filter.h"
#include <string.h>

// Bessel IIR order 2, Fc = 2.2 kHz, Fs = 2.4576 MHz (bilinear transform, prewarped)
// Measured on generated waveform: phase transition tr(10-90%) = 100 us
// (T.001 §2.3 Fig 2.6, ELT(DT): 50-150 us). DC gain = 1, -0.37 dB @ 800 Hz.
// Direct Form I: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]

static const float b0 = 1.2718166211e-05f;
static const float b1 = 2.5436332423e-05f;
static const float b2 = 1.2718166211e-05f;
static const float a1 = -1.9876334359f;
static const float a2 = 0.9876843086f;

void bessel_init(bessel_state_t *state) {
    // Clear state variables (Direct Form II)
    state->x1_i = 0.0f;
    state->x2_i = 0.0f;
    state->y1_i = 0.0f;
    state->y2_i = 0.0f;

    state->x1_q = 0.0f;
    state->x2_q = 0.0f;
    state->y1_q = 0.0f;
    state->y2_q = 0.0f;
}

void bessel_process(bessel_state_t *state, const iq_sample_t *input,
                    iq_sample_t *output, size_t num_samples) {

    for (size_t n = 0; n < num_samples; n++) {
        float x_i = (float)input[n].i;
        float x_q = (float)input[n].q;

        // IIR filter for I channel (Direct Form II)
        float y_i = b0*x_i + b1*state->x1_i + b2*state->x2_i
                    - a1*state->y1_i - a2*state->y2_i;

        // IIR filter for Q channel
        float y_q = b0*x_q + b1*state->x1_q + b2*state->x2_q
                    - a1*state->y1_q - a2*state->y2_q;

        // Update state variables (shift register)
        state->x2_i = state->x1_i;
        state->x1_i = x_i;
        state->y2_i = state->y1_i;
        state->y1_i = y_i;

        state->x2_q = state->x1_q;
        state->x1_q = x_q;
        state->y2_q = state->y1_q;
        state->y1_q = y_q;

        // Output filtered sample
        output[n].i = (int16_t)y_i;
        output[n].q = (int16_t)y_q;
    }
}
