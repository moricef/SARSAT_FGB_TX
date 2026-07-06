#ifndef PLUTO_CONTROL_H
#define PLUTO_CONTROL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "biphase_modulator.h"

// =============================
// PlutoSDR Control via libiio
// =============================

// PlutoSDR configuration
#define PLUTO_URI "ip:192.168.2.1"  // Default PlutoSDR IP (USB)
#define PLUTO_BUFFER_SIZE 16384      // TX buffer size (samples)

// Frequency settings
#define PLUTO_FREQ_TRAINING 403000000ULL  // 403 MHz (training/exercise)
#define PLUTO_FREQ_REAL     406000000ULL  // 406 MHz (real emergency)

// Sample rate: exact multiple of 6400 Hz baseband for exact 400 baud timing.
#define PLUTO_SAMPLE_RATE 2457600  // 2.4576 MSPS

// TX gain settings (dBm)
#define PLUTO_GAIN_LOW  -10   // Low power for training
#define PLUTO_GAIN_HIGH 0     // Max power (+7 dBm typical for PlutoSDR)

// =============================
// PlutoSDR Handle (opaque)
// =============================

typedef struct pluto_ctx pluto_ctx_t;

// =============================
// Initialization & Configuration
// =============================

// Initialize PlutoSDR connection
pluto_ctx_t* pluto_init(const char *uri);

// Configure TX parameters
bool pluto_configure_tx(pluto_ctx_t *ctx,
                        uint64_t center_freq,
                        uint32_t sample_rate,
                        int tx_gain_db);

// Cleanup
void pluto_cleanup(pluto_ctx_t *ctx);

// =============================
// Transmission Functions
// =============================

// Transmit I/Q waveform (blocking)
bool pluto_transmit_iq(pluto_ctx_t *ctx,
                       const iq_sample_t *samples,
                       size_t num_samples);

// Transmit T.001 frame (complete waveform: carrier + data)
// This version configures TX before transmission (legacy)
bool pluto_transmit_t001_frame(pluto_ctx_t *ctx,
                               const uint8_t *frame_bits,
                               uint64_t center_freq,
                               int tx_gain_db);

// Transmit T.001 frame without reconfiguring TX
// Use this when TX is already configured
bool pluto_transmit_t001_frame_simple(pluto_ctx_t *ctx,
                                      const uint8_t *frame_bits);

// =============================
// Utility Functions
// =============================

// Check PlutoSDR connection status
bool pluto_is_connected(pluto_ctx_t *ctx);

// Get actual TX frequency (for verification)
uint64_t pluto_get_tx_frequency(pluto_ctx_t *ctx);

// Get actual sample rate
uint32_t pluto_get_sample_rate(pluto_ctx_t *ctx);

#endif // PLUTO_CONTROL_H
