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
./bin/sarsat_t001 -f 431975000 -m 0 -g -10 -t 60 -lat 42.95463 -lon 1.364479 -alt 1080
```

Exercise mode with real-time GPS:

```bash
./bin/sarsat_t001 -gps -gps-uart /dev/ttyS1 -f 431975000 -m 0 -g -10 -t 60
```

Options:

```text
-f <freq>        Frequency in Hz (default: 431975000)
-g <gain>        AD9361 TX hardware gain in dB (default: -10, 0 = max output)
-i <id>          Beacon ID in hex (default: 0x123456)
-m <mode>        Mode: 0=exercise, 1=test (default: 0)
-t <sec>         Wait time after each transmission (default: 60)
-lat <lat>       Latitude (default: 42.95463)
-lon <lon>       Longitude (default: 1.364479)
-alt <alt>       Altitude in meters (default: 1080)
-gps             Enable real-time GPS, overriding fixed lat/lon/alt
-gps-uart <dev>  GPS serial device (default: /dev/ttyS1)
-h               Show help
```

Note: `-t` is the sleep time after a blocking transmission. The start-to-start interval is therefore roughly `-t` plus the burst duration and margins.

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

# Emetteur COSPAS-SARSAT T.001

Emetteur de balise COSPAS-SARSAT T.001 de premiere generation base sur ADALM-Pluto, destine aux essais locaux et a la validation de decodeurs.

## Etat

- Compilation Linux avec `gcc` et `libiio`.
- Generation d'une trame longue T.001 de 144 bits avec validation BCH1/BCH2.
- Emission I/Q PlutoSDR en BPSK Biphase-L / Manchester.
- Frequence RF par defaut : `431975000 Hz` (`431.975 MHz`).
- Sample-rate Pluto : `2457600 sps`, soit exactement `6400 * 384`, pour un debit exact de 400 bauds.
- GPS temps reel disponible avec `-gps`.
- Le code GPIO PA/relais/LED existe, mais le chemin TX principal le laisse desactive a cause de problemes de droits.

## Securite

Ce logiciel peut piloter du materiel RF. Utiliser uniquement des frequences, puissances, antennes et environnements pour lesquels vous etes autorise.

Ne pas emettre sur les frequences de detresse 406 MHz sans equipement certifie et autorisation explicite. La valeur par defaut `431.975 MHz` est prevue pour les essais locaux.

## Compilation

```bash
sudo apt update
sudo apt install -y build-essential libiio-dev libiio-utils git
make clean && make
```

Generateur IQ hors-ligne :

```bash
make -f Makefile.generate_iq
```

## Utilisation

Commande de test local validee :

```bash
./bin/sarsat_t001 -f 431975000 -m 1 -g 0 -t 5
```

Test a puissance plus faible :

```bash
./bin/sarsat_t001 -f 431975000 -m 1 -g -10 -t 5
```

Mode exercice avec position fixe :

```bash
./bin/sarsat_t001 -f 431975000 -m 0 -g -10 -t 60 -lat 42.95463 -lon 1.364479 -alt 1080
```

Mode exercice avec GPS temps reel :

```bash
./bin/sarsat_t001 -gps -gps-uart /dev/ttyS1 -f 431975000 -m 0 -g -10 -t 60
```

Options :

```text
-f <freq>        Frequence en Hz (defaut : 431975000)
-g <gain>        Gain TX AD9361 en dB (defaut : -10, 0 = puissance max)
-i <id>          Identifiant balise en hexadecimal (defaut : 0x123456)
-m <mode>        Mode : 0=exercice, 1=test (defaut : 0)
-t <sec>         Attente apres chaque emission (defaut : 60)
-lat <lat>       Latitude (defaut : 42.95463)
-lon <lon>       Longitude (defaut : 1.364479)
-alt <alt>       Altitude en metres (defaut : 1080)
-gps             Active le GPS temps reel
-gps-uart <dev>  Port serie GPS (defaut : /dev/ttyS1)
-h               Aide
```

## Generation IQ

```bash
bin/generate_iq_file -o /tmp/sarsat_t001.iq -s 40000
```

Le generateur utilise le meme chemin `build_t001_frame()` que l'emetteur, afin d'eviter les trames codees en dur obsoletes.

## Licence

MIT. Voir [LICENSE](LICENSE).
