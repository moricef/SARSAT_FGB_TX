# COSPAS-SARSAT T.001 Beacon Transmitter

Simulateur d'émetteur de balise COSPAS-SARSAT T.001 (1ère génération) pour formation ADRASEC.

## 📋 Caractéristiques

### Protocole T.001
- **Modulation** : BPSK (Biphase-L / Manchester)
- **Débit** : 400 baud
- **Fréquence** : 403 MHz (formation) / 406 MHz (réel)
- **Trame** : 144 bits (15 préambule + 9 sync + 120 données + BCH)
- **Timing** : 160 ms porteuse + 360 ms données

### Hardware
- **Platform** : Odroid C2 / Odroid C4 (ARM64, Armbian)
- **SDR** : ADALM-PLUTO (PlutoSDR) via USB
- **RF** : PA 5W + relais TX/RX + LEDs

### Signaux générés
- **Baseband** : 6400 Hz (16 samples/bit)
- **PlutoSDR** : 2.5 MSPS (interpolation x390.625)
- **I/Q** : BPSK ±1.1 rad (conforme T.001, I=A·cos(φ), Q=A·sin(φ))

## 🔧 Dépendances

### Logicielles
```bash
sudo apt update
sudo apt install -y build-essential libiio-dev libiio-utils git
```

### Matérielles
- PlutoSDR connecté via USB
- GPIO Odroid configurés :
  - GPIO 230 : PA enable
  - GPIO 231 : Relais TX/RX
  - GPIO 232 : LED status
  - GPIO 233 : LED TX

## 🚀 Compilation

```bash
cd /home/fab2/Developpement/COSPAS-SARSAT/ADALM-PLUTO/SARSAT_T001
make clean && make
```

## 📡 Utilisation

### Mode test (403 MHz, faible puissance)
```bash
sudo ./bin/sarsat_t001 -f 403000000 -g -10 -m 1 -t 120
```

### Mode exercice (403 MHz, GPS temps réel)
```bash
sudo ./bin/sarsat_t001 -f 403000000 -g 0 -m 0 -lat 48.8566 -lon 2.3522 -alt 35
```

### Options
```
-f <freq>     Fréquence en Hz (défaut: 403000000)
-g <gain>     Gain TX en dB (défaut: -10)
-i <id>       Beacon ID en hexa (défaut: 0x123456)
-m <mode>     Mode: 0=exercice, 1=test (défaut: 0)
-t <sec>      Intervalle TX en secondes (défaut: 60)
-lat <lat>    Latitude (défaut: 42.95463)
-lon <lon>    Longitude (défaut: 1.364479)
-alt <alt>    Altitude en mètres (défaut: 1080)
-h            Aide
```

## 🔍 Validation du signal

### Vérification PlutoSDR
```bash
iio_info -u ip:192.168.2.1
```

### Réception avec SDR
- **NFM** : 403.037 MHz, BW 12.5 kHz
- **USB** : 403.0365 MHz
- **Décodeur** : SARBeacon, Cospas-Sarsat Decoders

### Analyse spectrale
```bash
iio_readdev -u ip:192.168.2.1 -s 2500000 cf-ad9361-lpc | csdr ...
```

## 📂 Structure du projet

```
SARSAT_T001/
├── src/
│   ├── main.c                  # Application principale
│   ├── t001_protocol.c         # Protocole T.001 (BCH, GPS, trame)
│   ├── biphase_modulator.c     # Modulation Biphase-L + I/Q
│   ├── pluto_control.c         # Contrôle PlutoSDR (libiio)
│   └── gpio_control.c          # Contrôle GPIO (PA, relais, LEDs)
├── include/
│   ├── t001_protocol.h
│   ├── biphase_modulator.h
│   ├── pluto_control.h
│   └── gpio_control.h
├── Makefile
└── README.md
```

## 🧪 Tests

### Test 1: Validation protocole
```bash
./bin/sarsat_t001 -h  # Affiche la trame générée
```

### Test 2: Transmission simple
```bash
sudo ./bin/sarsat_t001 -f 403000000 -g -10 -m 1 -t 5
```

### Test 3: Validation BCH
Le programme vérifie automatiquement les codes BCH avant transmission :
- BCH1 (21 bits) pour PDF-1 (bits 25-85)
- BCH2 (12 bits) pour PDF-2 (bits 107-132)

## 📊 Spécifications techniques

### T.001 Frame Structure
| Bits | Champ | Description |
|------|-------|-------------|
| 1-15 | Preamble | 15 bits à 1 (détection porteuse) |
| 16-24 | Sync | 000101111 (normal) / 011010000 (test) |
| 25 | Format | 1 = message long |
| 26 | Protocol | 0 = protocole localisation |
| 27-36 | Country | Code pays (227 = France) |
| 37-40 | Protocol | Type balise (9 = ELT-DT) |
| 41-66 | Beacon ID | Identifiant unique 26 bits |
| 67-85 | Position | Position GPS 19 bits (30 min res) |
| 86-106 | BCH1 | Code correcteur 21 bits |
| 107-108 | Activation | Type d'activation |
| 109-112 | Altitude | Code altitude 4 bits |
| 113-114 | Freshness | Âge position |
| 115-132 | Offset | Offset position 18 bits (4 sec res) |
| 133-144 | BCH2 | Code correcteur 12 bits |

### BCH Polynomials (T.001 Annex B)
- **BCH1** : `0x26D9E3` (22 bits, degree 21)
- **BCH2** : `0x1539` (13 bits, degree 12)

## 🔒 Sécurité

⚠️ **ATTENTION** :
- **403 MHz** : Autorisé uniquement pour formation/exercice
- **406 MHz** : Réservé aux balises homologuées (urgences réelles)
- Respecter la réglementation ANFR (France) / FCC (USA)
- Ne jamais émettre sur 406 MHz sans autorisation

## 📚 Références

- **COSPAS-SARSAT T.001** : Specification for COSPAS-SARSAT 406 MHz Distress Beacons
- **PlutoSDR** : Analog Devices ADALM-PLUTO
- **libiio** : Analog Devices IIO Library

## 🐛 Dépannage

### PlutoSDR non détecté
```bash
sudo iio_info
# Vérifier USB : lsusb | grep "Analog Devices"
```

### Erreur GPIO
```bash
# Vérifier les numéros GPIO pour votre Odroid
ls /sys/class/gpio/
```

### Signal faible
- Vérifier PA enable (GPIO 230)
- Vérifier relais TX/RX (GPIO 231)
- Augmenter gain TX : `-g 0` (max +7 dBm PlutoSDR)

## 📝 TODO

## 🤝 Contribution

## 📄 Licence

Ce projet est distribué sous licence MIT. Voir [LICENSE](LICENSE).
