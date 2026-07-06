#include "pluto_control.h"
#include <iio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// =============================
// PlutoSDR Context Structure
// =============================

struct pluto_ctx {
    struct iio_context *ctx;
    struct iio_device *tx_dev;
    struct iio_channel *tx_i;
    struct iio_channel *tx_q;
    struct iio_buffer *txbuf;
};

// =============================
// Helper Functions
// =============================

static struct iio_channel* get_channel(struct iio_device *dev, const char *id) {
    struct iio_channel *chn = iio_device_find_channel(dev, id, true);
    if (!chn) {
        fprintf(stderr, "Channel %s not found\n", id);
    }
    return chn;
}

static bool set_param(struct iio_channel *chn, const char *attr, long long val) {
    int ret = iio_channel_attr_write_longlong(chn, attr, val);
    if (ret < 0) {
        fprintf(stderr, "Failed to set %s: %s\n", attr, strerror(-ret));
        return false;
    }
    return true;
}

// =============================
// Initialization & Configuration
// =============================

pluto_ctx_t* pluto_init(const char *uri) {
    pluto_ctx_t *pluto = calloc(1, sizeof(pluto_ctx_t));
    if (!pluto) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // Create IIO context
    if (uri) {
        pluto->ctx = iio_create_context_from_uri(uri);
    } else {
        pluto->ctx = iio_create_default_context();
    }

    if (!pluto->ctx) {
        fprintf(stderr, "Failed to create IIO context\n");
        free(pluto);
        return NULL;
    }

    // Get TX device (ad9361-phy for configuration, cf-ad9361-dds-core-lpc for TX)
    struct iio_device *phy = iio_context_find_device(pluto->ctx, "ad9361-phy");
    if (!phy) {
        fprintf(stderr, "ad9361-phy not found\n");
        iio_context_destroy(pluto->ctx);
        free(pluto);
        return NULL;
    }

    pluto->tx_dev = iio_context_find_device(pluto->ctx, "cf-ad9361-dds-core-lpc");
    if (!pluto->tx_dev) {
        fprintf(stderr, "TX device not found\n");
        iio_context_destroy(pluto->ctx);
        free(pluto);
        return NULL;
    }

    // Get TX I/Q channels
    pluto->tx_i = get_channel(pluto->tx_dev, "voltage0");
    pluto->tx_q = get_channel(pluto->tx_dev, "voltage1");

    if (!pluto->tx_i || !pluto->tx_q) {
        iio_context_destroy(pluto->ctx);
        free(pluto);
        return NULL;
    }

    // Enable TX channels
    iio_channel_enable(pluto->tx_i);
    iio_channel_enable(pluto->tx_q);

    printf("PlutoSDR initialized successfully\n");
    return pluto;
}

bool pluto_configure_tx(pluto_ctx_t *pluto,
                        uint64_t center_freq,
                        uint32_t sample_rate,
                        int tx_gain_db) {
    if (!pluto || !pluto->ctx) {
        return false;
    }

    struct iio_device *phy = iio_context_find_device(pluto->ctx, "ad9361-phy");
    if (!phy) {
        return false;
    }

    // Get TX LO channel
    struct iio_channel *tx_lo = iio_device_find_channel(phy, "altvoltage1", true);
    if (!tx_lo) {
        fprintf(stderr, "TX LO channel not found\n");
        return false;
    }

    // Set TX frequency
    if (!set_param(tx_lo, "frequency", center_freq)) {
        return false;
    }

    // Set sample rate
    struct iio_channel *tx_chan = iio_device_find_channel(phy, "voltage0", true);
    if (!tx_chan) {
        fprintf(stderr, "TX channel not found in phy\n");
        return false;
    }

    if (!set_param(tx_chan, "sampling_frequency", sample_rate)) {
        return false;
    }

    // AD9361 exposes TX hardwaregain in dB. Values are typically negative,
    // with 0 dB being maximum output power.
    if (!set_param(tx_chan, "hardwaregain", tx_gain_db)) {
        return false;
    }

    printf("PlutoSDR TX configured: freq=%llu Hz, rate=%u sps, gain=%d dB\n",
           (unsigned long long)center_freq, sample_rate, tx_gain_db);

    return true;
}

void pluto_cleanup(pluto_ctx_t *pluto) {
    if (!pluto) return;

    // Disable TX channels first to stop transmission
    if (pluto->tx_i) {
        iio_channel_disable(pluto->tx_i);
    }
    if (pluto->tx_q) {
        iio_channel_disable(pluto->tx_q);
    }

    if (pluto->txbuf) {
        iio_buffer_destroy(pluto->txbuf);
    }

    if (pluto->ctx) {
        iio_context_destroy(pluto->ctx);
    }

    free(pluto);
}

// =============================
// Transmission Functions
// =============================

bool pluto_transmit_iq(pluto_ctx_t *pluto,
                       const iq_sample_t *samples,
                       size_t num_samples) {
    if (!pluto || !pluto->tx_dev || !samples || num_samples == 0) {
        return false;
    }

    // Create TX buffer
    pluto->txbuf = iio_device_create_buffer(pluto->tx_dev, num_samples, false);
    if (!pluto->txbuf) {
        fprintf(stderr, "Failed to create TX buffer\n");
        return false;
    }

    // Fill buffer with I/Q samples (interleaved format: I, Q, I, Q, ...)
    int16_t *buf = (int16_t *)iio_buffer_start(pluto->txbuf);
    for (size_t i = 0; i < num_samples; i++) {
        buf[2*i]     = samples[i].i;  // I sample
        buf[2*i + 1] = samples[i].q;  // Q sample
    }

    // Push buffer to PlutoSDR
    ssize_t nbytes_tx = iio_buffer_push(pluto->txbuf);
    if (nbytes_tx < 0) {
        fprintf(stderr, "TX buffer push failed: %s\n", strerror(-nbytes_tx));
        iio_buffer_destroy(pluto->txbuf);
        pluto->txbuf = NULL;
        return false;
    }

    printf("Transmitted %zu I/Q samples (%zd bytes)\n", num_samples, nbytes_tx);

    // Wait for transmission to complete (duration + margin)
    usleep((num_samples * 1000000ULL / PLUTO_SAMPLE_RATE) + 50000);

    // Cleanup buffer
    iio_buffer_destroy(pluto->txbuf);
    pluto->txbuf = NULL;

    return true;
}

bool pluto_transmit_t001_frame(pluto_ctx_t *pluto,
                               const uint8_t *frame_bits,
                               uint64_t center_freq,
                               int tx_gain_db) {
    if (!pluto || !frame_bits) {
        return false;
    }

    // Configure TX
    if (!pluto_configure_tx(pluto, center_freq, PLUTO_SAMPLE_RATE, tx_gain_db)) {
        fprintf(stderr, "Failed to configure TX\n");
        return false;
    }

    // Generate T.001 waveform
    iq_sample_t *waveform = NULL;
    size_t waveform_length = 0;

    generate_t001_waveform(frame_bits, &waveform, &waveform_length);

    if (!waveform || waveform_length == 0) {
        fprintf(stderr, "Failed to generate T.001 waveform\n");
        return false;
    }

    printf("Generated T.001 waveform: %zu samples (%.1f ms)\n",
           waveform_length, (waveform_length * 1000.0) / PLUTO_SAMPLE_RATE);

    // Transmit waveform
    bool result = pluto_transmit_iq(pluto, waveform, waveform_length);

    // Cleanup
    free_t001_waveform(waveform);

    return result;
}

bool pluto_transmit_t001_frame_simple(pluto_ctx_t *pluto,
                                      const uint8_t *frame_bits) {
    if (!pluto || !frame_bits) {
        return false;
    }

    // Generate T.001 waveform
    iq_sample_t *waveform = NULL;
    size_t waveform_length = 0;

    generate_t001_waveform(frame_bits, &waveform, &waveform_length);

    if (!waveform || waveform_length == 0) {
        fprintf(stderr, "Failed to generate T.001 waveform\n");
        return false;
    }

    printf("Generated T.001 waveform: %zu samples (%.1f ms)\n",
           waveform_length, (waveform_length * 1000.0) / PLUTO_SAMPLE_RATE);

    // Transmit waveform
    bool result = pluto_transmit_iq(pluto, waveform, waveform_length);

    // Cleanup
    free_t001_waveform(waveform);

    return result;
}

// =============================
// Utility Functions
// =============================

bool pluto_is_connected(pluto_ctx_t *pluto) {
    return (pluto != NULL && pluto->ctx != NULL);
}

uint64_t pluto_get_tx_frequency(pluto_ctx_t *pluto) {
    if (!pluto || !pluto->ctx) {
        return 0;
    }

    struct iio_device *phy = iio_context_find_device(pluto->ctx, "ad9361-phy");
    if (!phy) {
        return 0;
    }

    struct iio_channel *tx_lo = iio_device_find_channel(phy, "altvoltage1", true);
    if (!tx_lo) {
        return 0;
    }

    long long freq = 0;
    iio_channel_attr_read_longlong(tx_lo, "frequency", &freq);
    return (uint64_t)freq;
}

uint32_t pluto_get_sample_rate(pluto_ctx_t *pluto) {
    if (!pluto || !pluto->ctx) {
        return 0;
    }

    struct iio_device *phy = iio_context_find_device(pluto->ctx, "ad9361-phy");
    if (!phy) {
        return 0;
    }

    struct iio_channel *tx_chan = iio_device_find_channel(phy, "voltage0", true);
    if (!tx_chan) {
        return 0;
    }

    long long rate = 0;
    iio_channel_attr_read_longlong(tx_chan, "sampling_frequency", &rate);
    return (uint32_t)rate;
}
