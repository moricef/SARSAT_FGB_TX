#ifndef T001_PROTOCOL_H
#define T001_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// =============================
// COSPAS-SARSAT T.001 Protocol Constants
// =============================

// Frame structure
#define MESSAGE_BITS 144
#define SAMPLES_PER_BIT 16
#define BASEBAND_SAMPLE_RATE 6400  // Hz (400 baud * 16 samples/bit)

// BCH polynomials (T.001 Annex B compliant)
#define BCH1_POLY       0x26D9E3  // 22-bit (X^21 + ... + 1)
#define BCH1_POLY_MASK  0x3FFFFF  // 22-bit mask
#define BCH1_DEGREE     21
#define BCH1_DATA_BITS  61

#define BCH2_POLY       0x1539    // 13-bit (X^12 + ... + 1)
#define BCH2_POLY_MASK  0x1FFF    // 13-bit working register; remainder fits in 12 bits.
                                  // 0x0FFF leaked a spurious bit 12 on ~50% of inputs,
                                  // breaking validate_t001_frame against the 12-bit field.
#define BCH2_DEGREE     12
#define BCH2_DATA_BITS  26

// Frame sync patterns
#define SYNC_NORMAL_LONG 0x02F  // 000101111 (9 bits MSB-first)
#define SYNC_SELF_TEST   0x0D0  // 011010000 (9 bits MSB-first)

// Country code (France)
#define COUNTRY_CODE_FRANCE 227

// Protocol type (ELT-DT)
#define PROTOCOL_ELT_DT 0x9  // 1001 binary

// Beacon modes
#define BEACON_MODE_EXERCISE 0
#define BEACON_MODE_TEST 1

// T.001 bit position macro (Bit 1 = index 0)
#define CS_BIT(bit_num) ((bit_num) - 1)

// Frame field positions (T.001 compliant)
#define FRAME_PREAMBLE_START 1
#define FRAME_PREAMBLE_LENGTH 15
#define FRAME_SYNC_START 16
#define FRAME_SYNC_LENGTH 9
#define FRAME_FORMAT_FLAG_BIT 25
#define FRAME_PROTOCOL_FLAG_BIT 26
#define FRAME_COUNTRY_START 27
#define FRAME_COUNTRY_LENGTH 10
#define FRAME_PROTOCOL_START 37
#define FRAME_PROTOCOL_LENGTH 4
#define FRAME_BEACON_ID_START 41
#define FRAME_BEACON_ID_LENGTH 26
#define FRAME_POSITION_START 67
#define FRAME_POSITION_LENGTH 19
#define FRAME_BCH1_START 86
#define FRAME_BCH1_LENGTH 21
#define FRAME_ACTIVATION_START 107
#define FRAME_ACTIVATION_LENGTH 2
#define FRAME_ALTITUDE_START 109
#define FRAME_ALTITUDE_LENGTH 4
#define FRAME_FRESHNESS_START 113
#define FRAME_FRESHNESS_LENGTH 2
#define FRAME_OFFSET_START 115
#define FRAME_OFFSET_LENGTH 18
#define FRAME_BCH2_START 133
#define FRAME_BCH2_LENGTH 12

// =============================
// Data Structures
// =============================

// GPS position encoding
typedef struct {
    uint64_t full_position_40bit;
    uint32_t coarse_position_21bit;
    uint32_t fine_position_19bit;
    uint32_t offset_position_18bit;
} gps_position_t;

// Beacon configuration
typedef struct {
    double latitude;
    double longitude;
    double altitude;
    uint32_t beacon_id;
    uint8_t mode;  // BEACON_MODE_EXERCISE or BEACON_MODE_TEST
} beacon_config_t;

// =============================
// BCH Functions
// =============================

uint32_t compute_bch(uint64_t data, int num_bits, uint32_t poly,
                     int poly_degree, uint32_t poly_mask);
uint32_t compute_bch1(uint64_t data);
uint16_t compute_bch2(uint32_t data);

// =============================
// GPS Encoding Functions
// =============================

gps_position_t encode_gps_position(double lat, double lon);
uint32_t compute_30min_position(double lat, double lon);
uint32_t compute_4sec_offset(double lat, double lon, uint32_t position_30min);
uint8_t altitude_to_code(double altitude);

// =============================
// Bit Operations
// =============================

void set_bit_field(uint8_t *frame, uint16_t cs_start_bit, uint8_t length, uint64_t value);
uint64_t get_bit_field(const uint8_t *frame, uint16_t cs_start_bit, uint8_t length);

// =============================
// Frame Construction
// =============================

void build_t001_frame(uint8_t *frame, const beacon_config_t *config);
bool validate_t001_frame(const uint8_t *frame);

// =============================
// Debug Functions
// =============================

void print_frame_hex(const uint8_t *frame);
void print_frame_analysis(const uint8_t *frame);

#endif // T001_PROTOCOL_H
