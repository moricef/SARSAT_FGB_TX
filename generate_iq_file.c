/*
 * Générateur de fichier IQ COSPAS-SARSAT T.001
 * Utilise les fonctions du projet SARSAT_FGB pour générer un fichier .iq
 * Format: complex64 (float32 I/Q entrelacés), sample rate 40000 Hz
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "t001_protocol.h"
#include "biphase_modulator.h"

void print_usage(const char *progname) {
    printf("Usage: %s [options]\n\n", progname);
    printf("Options:\n");
    printf("  -o <file>       Output IQ file (default: output.iq)\n");
    printf("  -s <rate>       Output sample rate Hz (default: 40000)\n");
    printf("  -i <id>         Beacon ID in hex (default: 0x123456)\n");
    printf("  -m <mode>       Mode: 0=exercise, 1=test (default: 1)\n");
    printf("  -lat <lat>      Latitude (default: 42.95463)\n");
    printf("  -lon <lon>      Longitude (default: 1.364479)\n");
    printf("  -alt <alt>      Altitude in meters (default: 1080)\n");
    printf("  -h              Show this help\n\n");
    printf("Génère un fichier IQ COSPAS-SARSAT T.001 (144 bits)\n");
    printf("Format: complex64 (float32 I/Q), compatible GNU Radio\n\n");
}

int main(int argc, char **argv) {
    const char *output_file = "output.iq";
    int output_sample_rate = 40000;
    beacon_config_t cfg = {
        .latitude = 42.95463,
        .longitude = 1.364479,
        .altitude = 1080,
        .beacon_id = 0x123456,
        .mode = BEACON_MODE_TEST
    };

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            output_sample_rate = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            cfg.beacon_id = strtoul(argv[++i], NULL, 16);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            cfg.mode = (uint8_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-lat") == 0 && i + 1 < argc) {
            cfg.latitude = atof(argv[++i]);
        } else if (strcmp(argv[i], "-lon") == 0 && i + 1 < argc) {
            cfg.longitude = atof(argv[++i]);
        } else if (strcmp(argv[i], "-alt") == 0 && i + 1 < argc) {
            cfg.altitude = atof(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    printf("=======================================================\n");
    printf("  Générateur de fichier IQ COSPAS-SARSAT T.001\n");
    printf("=======================================================\n\n");

    iq_sample_t *waveform = NULL;
    size_t waveform_length = 0;

    // Build the frame with the same code path as the transmitter (single
    // source of truth — a hardcoded bit array here went stale and its
    // displayed hex no longer matched the actual bits).
    uint8_t frame[MESSAGE_BITS];
    printf("[1/5] Génération de la trame T.001 (144 bits)...\n");
    build_t001_frame(frame, &cfg);
    if (!validate_t001_frame(frame)) {
        fprintf(stderr, "Erreur: BCH validation failed\n");
        return 1;
    }
    printf("      ");
    print_frame_hex(frame);

    printf("[2/5] Génération de la forme d'onde (%u sps)...\n", PLUTO_SAMPLE_RATE);
    generate_t001_waveform(frame, &waveform, &waveform_length);

    if (!waveform || waveform_length == 0) {
        fprintf(stderr, "Erreur: Impossible de générer la forme d'onde\n");
        return 1;
    }

    printf("      Échantillons générés: %zu (int16 I/Q)\n", waveform_length);
    printf("      Durée: %.3f s\n", (double)waveform_length / PLUTO_SAMPLE_RATE);

    // Décimation/Interpolation si nécessaire
    printf("[3/5] Rééchantillonnage vers %d Hz...\n", output_sample_rate);

    size_t output_length = (waveform_length * output_sample_rate) / PLUTO_SAMPLE_RATE;

    // Ajouter quelques échantillons pour dépasser MIN_SAMPLES_FOR_FRAME (20800)
    if (output_length < 20850) {
        output_length = 20850;
    }

    // Conversion vers float32 I/Q
    printf("[4/5] Conversion vers format complex64 (float32)...\n");

    float *output_iq = malloc(output_length * 2 * sizeof(float));
    if (!output_iq) {
        fprintf(stderr, "Erreur: Impossible d'allouer la mémoire de sortie\n");
        free_t001_waveform(waveform);
        return 1;
    }

    // Rééchantillonner et convertir (interpolation linéaire simple)
    for (size_t i = 0; i < output_length; i++) {
        // Position dans la forme d'onde source
        double src_pos = ((double)i * PLUTO_SAMPLE_RATE) / output_sample_rate;
        size_t idx = (size_t)src_pos;

        if (idx >= waveform_length) idx = waveform_length - 1;

        // Normaliser int16 vers float [-1.0, 1.0] et augmenter l'amplitude
        // Multiplier par 3 pour avoir ~0.15 (au-dessus du seuil 0.05 du démodulateur)
        output_iq[i * 2 + 0] = 3.0f * (float)waveform[idx].i / 32768.0f;  // I
        output_iq[i * 2 + 1] = 3.0f * (float)waveform[idx].q / 32768.0f;  // Q
    }

    printf("      Échantillons de sortie: %zu\n", output_length);
    printf("      Durée: %.3f s\n", (double)output_length / output_sample_rate);

    // Écrire le fichier
    printf("[5/5] Écriture du fichier: %s\n", output_file);

    FILE *fp = fopen(output_file, "wb");
    if (!fp) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir %s\n", output_file);
        free(output_iq);
        free_t001_waveform(waveform);
        return 1;
    }

    size_t written = fwrite(output_iq, sizeof(float), output_length * 2, fp);
    fclose(fp);

    if (written != output_length * 2) {
        fprintf(stderr, "Erreur: Écriture incomplète (%zu/%zu)\n", written, output_length * 2);
        free(output_iq);
        free_t001_waveform(waveform);
        return 1;
    }

    printf("\n");
    printf("✓ Fichier IQ généré avec succès!\n\n");
    printf("Informations:\n");
    printf("  - Fichier: %s\n", output_file);
    printf("  - Taille: %.2f Ko\n", (written * sizeof(float)) / 1024.0);
    printf("  - Format: complex64 (float32 I/Q)\n");
    printf("  - Sample rate: %d Hz\n", output_sample_rate);
    printf("  - Échantillons: %zu\n", output_length);
    printf("  - Durée: %.3f s\n", (double)output_length / output_sample_rate);
    printf("\n");
    printf("Test avec GNU Radio:\n");
    printf("  python3 test_demodulator.py %s %d\n", output_file, output_sample_rate);
    printf("\n");
    printf("Visualisation:\n");
    printf("  python3 visualize_iq.py %s\n", output_file);
    printf("\n");

    // Cleanup
    free(output_iq);
    free_t001_waveform(waveform);

    return 0;
}
