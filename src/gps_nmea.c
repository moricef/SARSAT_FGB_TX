#include "gps_nmea.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/select.h>

// =============================
// UART Communication
// =============================

gps_ctx_t* gps_init(const char *uart_device) {
    gps_ctx_t *ctx = calloc(1, sizeof(gps_ctx_t));
    if (!ctx) {
        fprintf(stderr, "GPS: Failed to allocate context\n");
        return NULL;
    }

    // Set UART device path
    if (uart_device) {
        strncpy(ctx->uart_device, uart_device, sizeof(ctx->uart_device) - 1);
    } else {
        strncpy(ctx->uart_device, GPS_UART_DEFAULT, sizeof(ctx->uart_device) - 1);
    }

    // Open UART
    ctx->fd = open(ctx->uart_device, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (ctx->fd < 0) {
        fprintf(stderr, "GPS: Failed to open %s: %s\n", ctx->uart_device, strerror(errno));
        free(ctx);
        return NULL;
    }

    // Configure UART
    struct termios tty;
    if (tcgetattr(ctx->fd, &tty) != 0) {
        fprintf(stderr, "GPS: Failed to get tty attributes: %s\n", strerror(errno));
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    // Set baud rate
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    // 8N1 mode, no flow control
    tty.c_cflag &= ~PARENB;        // No parity
    tty.c_cflag &= ~CSTOPB;        // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            // 8 data bits
    tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines

    // Raw input mode
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // Raw output mode
    tty.c_oflag &= ~OPOST;

    // Read settings
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(ctx->fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "GPS: Failed to set tty attributes: %s\n", strerror(errno));
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    // Flush input buffer
    tcflush(ctx->fd, TCIFLUSH);

    ctx->initialized = true;
    ctx->data.position_valid = false;
    ctx->data.time_valid = false;

    printf("GPS: Initialized on %s at %d baud\n", ctx->uart_device, GPS_BAUD_RATE);
    return ctx;
}

void gps_cleanup(gps_ctx_t *ctx) {
    if (!ctx) return;

    if (ctx->fd >= 0) {
        close(ctx->fd);
    }

    free(ctx);
}

// =============================
// NMEA Checksum Validation
// =============================

bool nmea_validate_checksum(const char *sentence) {
    if (!sentence || sentence[0] != '$') {
        return false;
    }

    // Find checksum delimiter
    const char *star = strchr(sentence, '*');
    if (!star || strlen(star) < 3) {
        return false;
    }

    // Calculate checksum (XOR of all bytes between $ and *)
    uint8_t calc_checksum = 0;
    for (const char *p = sentence + 1; p < star; p++) {
        calc_checksum ^= (uint8_t)*p;
    }

    // Parse received checksum
    uint8_t recv_checksum = 0;
    if (sscanf(star + 1, "%2hhx", &recv_checksum) != 1) {
        return false;
    }

    return calc_checksum == recv_checksum;
}

// =============================
// NMEA Coordinate Parsing
// =============================

static double nmea_parse_coordinate(const char *coord, const char *dir) {
    if (!coord || !dir || strlen(coord) == 0) {
        return 0.0;
    }

    // Format: DDMM.MMMM or DDDMM.MMMM
    double raw = atof(coord);
    int degrees = (int)(raw / 100.0);
    double minutes = raw - (degrees * 100.0);
    double decimal = degrees + (minutes / 60.0);

    // Apply direction
    if (dir[0] == 'S' || dir[0] == 'W') {
        decimal = -decimal;
    }

    return decimal;
}

static void nmea_parse_time(const char *time_str, struct tm *tm_out) {
    if (!time_str || !tm_out || strlen(time_str) < 6) {
        return;
    }

    memset(tm_out, 0, sizeof(struct tm));

    // Format: HHMMSS.SSS
    char hour[3] = {0}, min[3] = {0}, sec[3] = {0};
    strncpy(hour, time_str, 2);
    strncpy(min, time_str + 2, 2);
    strncpy(sec, time_str + 4, 2);

    tm_out->tm_hour = atoi(hour);
    tm_out->tm_min = atoi(min);
    tm_out->tm_sec = atoi(sec);
}

// =============================
// GGA Sentence Parser
// =============================

bool nmea_parse_gga(const char *sentence, gps_data_t *data) {
    if (!sentence || !data) return false;
    if (!nmea_validate_checksum(sentence)) return false;

    // $GPGGA,time,lat,N/S,lon,E/W,quality,numSV,HDOP,alt,M,...*checksum
    char time_str[16] = {0};
    char lat_str[16] = {0}, lat_dir[2] = {0};
    char lon_str[16] = {0}, lon_dir[2] = {0};
    int quality = 0, satellites = 0;
    double hdop = 0.0, altitude = 0.0;
    char alt_unit[2] = {0};

    int parsed = sscanf(sentence, "$G%*cGGA,%[^,],%[^,],%[^,],%[^,],%[^,],%d,%d,%lf,%lf,%[^,]",
                       time_str, lat_str, lat_dir, lon_str, lon_dir,
                       &quality, &satellites, &hdop, &altitude, alt_unit);

    if (parsed < 9) {
        return false;
    }

    // Update GPS data
    data->latitude = nmea_parse_coordinate(lat_str, lat_dir);
    data->longitude = nmea_parse_coordinate(lon_str, lon_dir);
    data->altitude = altitude;
    data->fix_quality = quality;
    data->satellites = satellites;
    data->hdop = hdop;
    data->position_valid = (quality > 0);

    nmea_parse_time(time_str, &data->utc_time);
    data->time_valid = (strlen(time_str) >= 6);
    data->last_update = time(NULL);

    return true;
}

// =============================
// RMC Sentence Parser
// =============================

bool nmea_parse_rmc(const char *sentence, gps_data_t *data) {
    if (!sentence || !data) return false;
    if (!nmea_validate_checksum(sentence)) return false;

    // $GPRMC,time,status,lat,N/S,lon,E/W,speed,course,date,...*checksum
    char time_str[16] = {0};
    char status[2] = {0};
    char lat_str[16] = {0}, lat_dir[2] = {0};
    char lon_str[16] = {0}, lon_dir[2] = {0};
    double speed = 0.0, course = 0.0;
    char date_str[16] = {0};

    int parsed = sscanf(sentence, "$G%*cRMC,%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%lf,%lf,%[^,]",
                       time_str, status, lat_str, lat_dir, lon_str, lon_dir,
                       &speed, &course, date_str);

    if (parsed < 8) {
        return false;
    }

    // Only update if status is Active
    if (status[0] != 'A') {
        return false;
    }

    // Update GPS data
    data->latitude = nmea_parse_coordinate(lat_str, lat_dir);
    data->longitude = nmea_parse_coordinate(lon_str, lon_dir);
    data->speed_knots = speed;
    data->course_deg = course;
    data->position_valid = true;

    nmea_parse_time(time_str, &data->utc_time);
    data->time_valid = (strlen(time_str) >= 6);
    data->last_update = time(NULL);

    return true;
}

// =============================
// GPS Update Function
// =============================

bool gps_update(gps_ctx_t *ctx, int timeout_sec) {
    if (!ctx || !ctx->initialized) {
        return false;
    }

    char buffer[GPS_NMEA_MAX_LENGTH + 1];
    int buffer_pos = 0;
    bool got_valid_data = false;

    time_t start_time = time(NULL);

    while (1) {
        // Check timeout
        if (timeout_sec > 0 && (time(NULL) - start_time) >= timeout_sec) {
            break;
        }

        // Use select for timeout
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(ctx->fd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int ret = select(ctx->fd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            fprintf(stderr, "GPS: select error: %s\n", strerror(errno));
            return false;
        } else if (ret == 0) {
            // Timeout, continue waiting
            continue;
        }

        // Read available data
        char c;
        ssize_t n = read(ctx->fd, &c, 1);
        if (n <= 0) {
            continue;
        }

        // Build NMEA sentence
        if (c == '$') {
            buffer_pos = 0;
            buffer[buffer_pos++] = c;
        } else if (buffer_pos > 0 && buffer_pos < GPS_NMEA_MAX_LENGTH) {
            buffer[buffer_pos++] = c;

            // End of sentence
            if (c == '\n') {
                buffer[buffer_pos] = '\0';

                // Parse sentence
                if (strstr(buffer, "$GPGGA") || strstr(buffer, "$GNGGA")) {
                    if (nmea_parse_gga(buffer, &ctx->data)) {
                        got_valid_data = true;
                        if (timeout_sec == 0) return true; // Non-blocking mode
                    }
                } else if (strstr(buffer, "$GPRMC") || strstr(buffer, "$GNRMC")) {
                    if (nmea_parse_rmc(buffer, &ctx->data)) {
                        got_valid_data = true;
                        if (timeout_sec == 0) return true; // Non-blocking mode
                    }
                }

                buffer_pos = 0;
            }
        } else {
            buffer_pos = 0; // Reset on overflow
        }
    }

    return got_valid_data;
}

// =============================
// GPS Status Functions
// =============================

const gps_data_t* gps_get_data(const gps_ctx_t *ctx) {
    return ctx ? &ctx->data : NULL;
}

bool gps_has_fix(const gps_ctx_t *ctx) {
    if (!ctx) return false;
    return ctx->data.position_valid && ctx->data.fix_quality > 0;
}

void gps_print_status(const gps_ctx_t *ctx) {
    if (!ctx) {
        printf("GPS: Not initialized\n");
        return;
    }

    const gps_data_t *data = &ctx->data;

    printf("\n=== GPS Status ===\n");
    printf("Fix:        %s (quality: %d)\n",
           data->position_valid ? "VALID" : "INVALID", data->fix_quality);
    printf("Satellites: %d\n", data->satellites);
    printf("Position:   %.6f, %.6f\n", data->latitude, data->longitude);
    printf("Altitude:   %.1f m\n", data->altitude);
    printf("HDOP:       %.2f\n", data->hdop);
    printf("Speed:      %.2f knots\n", data->speed_knots);
    printf("Course:     %.1f°\n", data->course_deg);

    if (data->time_valid) {
        printf("UTC Time:   %02d:%02d:%02d\n",
               data->utc_time.tm_hour, data->utc_time.tm_min, data->utc_time.tm_sec);
    }

    time_t now = time(NULL);
    printf("Last Update: %ld seconds ago\n", now - data->last_update);
    printf("==================\n\n");
}
