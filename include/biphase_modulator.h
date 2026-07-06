#ifndef BIPHASE_MODULATOR_H
#define BIPHASE_MODULATOR_H

#include <stdint.h>
#include <stddef.h>

// =============================
// Biphase-L (Manchester) Modulator for T.001
// =============================

// Signal parameters
#define BAUD_RATE 400
#define SAMPLES_PER_BIT 16
#define BASEBAND_RATE 6400  // 400 * 16

// Carrier parameters (403 MHz for training, 406 MHz for real)
#define CARRIER_FREQUENCY_TRAINING 403000000ULL  // 403 MHz
#define CARRIER_FREQUENCY_REAL     406000000ULL  // 406 MHz

// Timing (T.001 standard)
#define CARRIER_DURATION_MS 160  // Unmodulated carrier
#define MESSAGE_DURATION_MS 360  // BPSK data

// PlutoSDR interpolation. Keep this an exact multiple of BASEBAND_RATE
// so 400 baud timing stays exact.
#define PLUTO_SAMPLE_RATE 2457600  // 2.4576 MSPS = 6400 * 384
#define INTERPOLATION_FACTOR (PLUTO_SAMPLE_RATE / BASEBAND_RATE)

// I/Q sample structure (16-bit signed for PlutoSDR)
typedef struct {
    int16_t i;
    int16_t q;
} iq_sample_t;

// =============================
// Biphase-L Encoding
// =============================

// Generate Biphase-L baseband samples (6400 Hz, 16 samples/bit)
// bit = 0 -> transition 0→1 (low first half, high second half)
// bit = 1 -> transition 1→0 (high first half, low second half)
void generate_biphase_baseband(const uint8_t *frame_bits, int num_bits,
                                int16_t *baseband, size_t *num_samples);

// =============================
// BPSK I/Q Modulation
// =============================

// Generate BPSK I/Q samples from baseband
// Phase: ±1.1 rad (T.001 spec: bit=0 -> -1.1 rad, bit=1 -> +1.1 rad)
// I(t) = A*cos(phase), Q(t) = A*sin(phase)
void modulate_bpsk_iq(const int16_t *baseband, size_t num_baseband_samples,
                      iq_sample_t *iq_samples, size_t *num_iq_samples);

// =============================
// Complete T.001 Transmission Waveform
// =============================

// Generate complete T.001 transmission:
// 1. Unmodulated carrier (160 ms)
// 2. BPSK data (360 ms @ 400 baud)
void generate_t001_waveform(const uint8_t *frame_bits,
                             iq_sample_t **waveform,
                             size_t *waveform_length);

// Free waveform memory
void free_t001_waveform(iq_sample_t *waveform);

#endif // BIPHASE_MODULATOR_H
