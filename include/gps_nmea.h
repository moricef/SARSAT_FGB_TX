#ifndef GPS_NMEA_H
#define GPS_NMEA_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// =============================
// GPS NMEA Constants
// =============================

#define GPS_UART_DEFAULT "/dev/ttyS1"
#define GPS_BAUD_RATE 9600
#define GPS_NMEA_MAX_LENGTH 82
#define GPS_READ_TIMEOUT_SEC 2

// Fix quality indicators
#define GPS_FIX_INVALID 0
#define GPS_FIX_GPS 1
#define GPS_FIX_DGPS 2
#define GPS_FIX_PPS 3
#define GPS_FIX_RTK 4
#define GPS_FIX_FLOAT_RTK 5
#define GPS_FIX_ESTIMATED 6
#define GPS_FIX_MANUAL 7
#define GPS_FIX_SIMULATION 8

// =============================
// Data Structures
// =============================

// GPS data structure
typedef struct {
    // Position
    double latitude;         // Decimal degrees (-90 to +90)
    double longitude;        // Decimal degrees (-180 to +180)
    double altitude;         // Meters above sea level

    // Fix quality
    uint8_t fix_quality;     // GPS fix quality (0=invalid, 1=GPS fix, 2=DGPS, etc.)
    uint8_t satellites;      // Number of satellites in use

    // Time
    struct tm utc_time;      // UTC time from GPS
    bool time_valid;

    // Velocity
    double speed_knots;      // Speed over ground in knots
    double course_deg;       // Course over ground in degrees

    // Accuracy
    double hdop;             // Horizontal dilution of precision

    // Status
    bool position_valid;     // True if position is valid
    time_t last_update;      // Timestamp of last GPS update
} gps_data_t;

// GPS context
typedef struct {
    int fd;                  // Serial port file descriptor
    char uart_device[256];   // UART device path
    gps_data_t data;         // Current GPS data
    bool initialized;        // Initialization status
} gps_ctx_t;

// =============================
// GPS Functions
// =============================

/**
 * Initialize GPS module
 * @param uart_device UART device path (e.g., "/dev/ttyS1"), NULL for default
 * @return GPS context, or NULL on failure
 */
gps_ctx_t* gps_init(const char *uart_device);

/**
 * Cleanup GPS module
 * @param ctx GPS context
 */
void gps_cleanup(gps_ctx_t *ctx);

/**
 * Read and parse GPS data (non-blocking with timeout)
 * @param ctx GPS context
 * @param timeout_sec Timeout in seconds (0 for non-blocking)
 * @return true if new valid data received, false otherwise
 */
bool gps_update(gps_ctx_t *ctx, int timeout_sec);

/**
 * Get current GPS data
 * @param ctx GPS context
 * @return Pointer to GPS data structure
 */
const gps_data_t* gps_get_data(const gps_ctx_t *ctx);

/**
 * Check if GPS has valid fix
 * @param ctx GPS context
 * @return true if GPS has valid fix
 */
bool gps_has_fix(const gps_ctx_t *ctx);

/**
 * Print GPS status
 * @param ctx GPS context
 */
void gps_print_status(const gps_ctx_t *ctx);

// =============================
// NMEA Parsing Functions (Internal)
// =============================

bool nmea_parse_gga(const char *sentence, gps_data_t *data);
bool nmea_parse_rmc(const char *sentence, gps_data_t *data);
bool nmea_validate_checksum(const char *sentence);

#endif // GPS_NMEA_H
