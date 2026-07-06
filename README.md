# COSPAS-SARSAT T.001 Beacon Transmitter

ADALM-Pluto based COSPAS-SARSAT T.001 first-generation beacon transmitter for local training and decoder validation.

## Status

- Builds on Linux with `gcc` and `libiio`.
- Generates a 144-bit T.001 long message with BCH1/BCH2 validation.
- Transmits Biphase-L / Manchester BPSK I/Q through PlutoSDR.
- Default RF frequency: `431975000 Hz` (`431.975 MHz`).
- Pluto sample rate: `2457600 sps`, exactly `6400 * 384`, so 400 baud timing is exact.
- Real-time GPS input is available with `-gps`.
- GPIO PA/relay/LED code exists, but the main TX path currently leaves GPIO disabled because of permission issues.

## Safety

This software can drive RF hardware. Use only frequencies, power levels, antennas, and environments for which you are authorized.

Do not transmit on 406 MHz distress-beacon frequencies unless you are using certified equipment and have explicit authorization. The default `431.975 MHz` value is intended for local test setups.

## Features

- T.001 long frame: 144 bits.
- Carrier phase: 160 ms unmodulated carrier, then 360 ms data.
- Modulation: BPSK with +/- 1.1 rad phase deviation.
- Baseband: 400 baud, 16 samples per bit, 6400 Hz.
- Pluto I/Q: 2.4576 MSPS, integer interpolation factor 384.
- BCH1 polynomial: `0x26D9E3`.
- BCH2 polynomial: `0x1539`, with a 13-bit working register mask `0x1FFF`; the transmitted field is 12 bits.
- GPS NMEA parser for GGA/RMC.
- Offline IQ generator: `bin/generate_iq_file`.

## Requirements

```bash
sudo apt update
sudo apt install -y build-essential libiio-dev libiio-utils git
```

Hardware:

- ADALM-Pluto / PlutoSDR reachable at `ip:192.168.2.1`.
- Optional GPS receiver exposed as a Linux serial device.
- Optional Odroid GPIO wiring for PA, TX/RX relay, and LEDs.

## Build

```bash
make clean && make
```

Build the offline IQ generator:

```bash
make -f Makefile.generate_iq
```

## Usage

Default configuration:

```bash
./bin/sarsat_t001
```

Known-good local test command:

```bash
./bin/sarsat_t001 -f 431975000 -m 1 -g 0 -t 5
```

Lower-power local test:

```bash
./bin/sarsat_t001 -f 431975000 -m 1 -g -10 -t 5
```

Exercise mode with fixed position:

```bash
./bin/sarsat_t001 -f 431975000 -m 0 -g -10 -t 50 -lat 42.95463 -lon 1.364479 -alt 1080
```

Exercise mode with real-time GPS:

```bash
./bin/sarsat_t001 -gps -gps-uart /dev/ttyS1 -f 431975000 -m 0 -g -10 -t 50
```

Options:

```text
-f <freq>        Frequency in Hz (default: 431975000)
-g <gain>        AD9361 TX hardware gain in dB (default: -10, 0 = max output)
-i <id>          Beacon ID in hex (default: 0x123456)
-m <mode>        Mode: 0=exercise, 1=test (default: 0)
-t <sec>         FGB burst period, fixed-phase schedule (default: 50)
-lat <lat>       Latitude (default: 42.95463)
-lon <lon>       Longitude (default: 1.364479)
-alt <alt>       Altitude in meters (default: 1080)
-gps             Enable real-time GPS, overriding fixed lat/lon/alt
-gps-uart <dev>  GPS serial device (default: /dev/ttyS1)
-homing <freq>   ELT swept-tone AM homing between exercise bursts at <freq> Hz
                 (exercise mode only; stops 5 s before each burst;
                 TX is primed once before the scheduled loop)
-h               Show help
```

Note: `-t` is a fixed start-to-start FGB burst period. With homing enabled,
the homing signal stops 5 s before the next scheduled FGB burst.

`-homing` generates an ELT-like swept-tone AM homing signal compatible with
published 121.5 MHz homing characteristics; not claimed as DO-183 certified.

## Offline IQ Generation

Generate a complex64 IQ file at 40 kS/s:

```bash
bin/generate_iq_file -o /tmp/sarsat_t001.iq -s 40000
```

The generator uses the same `build_t001_frame()` path as the transmitter, so its frame content stays aligned with the live TX code.

## Project Layout

```text
SARSAT_FGB/
├── src/
│   ├── main.c
│   ├── t001_protocol.c
│   ├── biphase_modulator.c
│   ├── bessel_filter.c
│   ├── pluto_control.c
│   ├── gpio_control.c
│   └── gps_nmea.c
├── include/
│   ├── t001_protocol.h
│   ├── biphase_modulator.h
│   ├── bessel_filter.h
│   ├── pluto_control.h
│   ├── gpio_control.h
│   └── gps_nmea.h
├── Makefile
├── Makefile.generate_iq
└── README.md
```

## Validation Notes

- `validate_t001_frame()` checks BCH1 and BCH2 before transmission.
- Recent local decoder runs at `431.975 MHz` showed stable FGB decoding with `CRC1=OK CRC2=OK` on the received bursts.
- The Bessel IIR is configured for a 2.2 kHz cutoff at 2.4576 MSPS and was tuned to keep the phase-transition rise time in the target range.

## License

MIT. See [LICENSE](LICENSE).

---

# Émetteur COSPAS-SARSAT T.001

Émetteur de balise COSPAS-SARSAT T.001 de première génération basé sur ADALM-Pluto, destiné aux essais locaux et à la validation de décodeurs.

## État

- Compilation Linux avec `gcc` et `libiio`.
- Génération d'une trame longue T.001 de 144 bits avec validation BCH1/BCH2.
- Émission I/Q PlutoSDR en BPSK Biphase-L / Manchester.
- Fréquence RF par défaut : `431975000 Hz` (`431.975 MHz`).
- Sample-rate Pluto : `2457600 sps`, soit exactement `6400 * 384`, pour un débit exact de 400 bauds.
- GPS temps réel disponible avec `-gps`.
- Le code GPIO PA/relais/LED existe, mais le chemin TX principal le laisse désactivé à cause de problèmes de droits.

## Sécurité

Ce logiciel peut piloter du matériel RF. Utiliser uniquement des fréquences, puissances, antennes et environnements pour lesquels vous êtes autorisés.

Ne pas émettre sur les fréquences de détresse 406 MHz sans équipement certifié et autorisation explicite. La valeur par défaut `431.975 MHz` est prévue pour les essais locaux.

## Compilation

```bash
sudo apt update
sudo apt install -y build-essential libiio-dev libiio-utils git
make clean && make
```

Générateur IQ hors-ligne :

```bash
make -f Makefile.generate_iq
```

## Utilisation

Commande de test local validée :

```bash
./bin/sarsat_t001 -f 431975000 -m 1 -g 0 -t 5
```

Test à puissance plus faible :

```bash
./bin/sarsat_t001 -f 431975000 -m 1 -g -10 -t 5
```

Mode exercice avec position fixe :

```bash
./bin/sarsat_t001 -f 431975000 -m 0 -g -10 -t 50 -lat 42.95463 -lon 1.364479 -alt 1080
```

Mode exercice avec GPS temps réel :

```bash
./bin/sarsat_t001 -gps -gps-uart /dev/ttyS1 -f 431975000 -m 0 -g -10 -t 50
```

Options :

```text
-f <freq>        Fréquence en Hz (défaut : 431975000)
-g <gain>        Gain TX AD9361 en dB (défaut : -10, 0 = puissance max)
-i <id>          Identifiant balise en hexadécimal (défaut : 0x123456)
-m <mode>        Mode : 0=exercice, 1=test (défaut : 0)
-t <sec>         Période des bursts FGB, cadence à phase fixe (défaut : 50)
-lat <lat>       Latitude (défaut : 42.95463)
-lon <lon>       Longitude (défaut : 1.364479)
-alt <alt>       Altitude en mètres (défaut : 1080)
-gps             Active le GPS temps réel
-gps-uart <dev>  Port série GPS (défaut : /dev/ttyS1)
-homing <freq>   Homing AM ELT à tonalité balayée entre les bursts d'exercice, à <freq> Hz
                 (mode exercice uniquement ; s'arrête 5 s avant chaque burst ;
                 le TX est amorcé une fois avant la boucle planifiée)
-h               Aide
```

Note : `-t` est une période fixe entre débuts de bursts FGB. Avec le homing
activé, le signal de homing s'arrête 5 s avant le burst FGB planifié suivant.

`-homing` génère un signal de homing AM à tonalité balayée de type ELT,
compatible avec les caractéristiques publiées du homing 121,5 MHz ; il n'est
pas revendiqué comme certifié DO-183.

## Génération IQ

```bash
bin/generate_iq_file -o /tmp/sarsat_t001.iq -s 40000
```

Le générateur utilise le même chemin `build_t001_frame()` que l'émetteur, afin d'éviter les trames codées en dur obsolètes.

## Licence

MIT. Voir [LICENSE](LICENSE).
