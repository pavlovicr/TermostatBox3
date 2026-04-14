# 🌡️ WiFi Sobni Termostat — ESP32-S3-Box-3 + Shelly 2PM Gen3

Pametni sobni termostat z zaslonom na dotik, ki temelji na ESP32-S3-Box-3 in prek WiFi omrežja krmili Shelly 2PM Gen3 stikalo za vklop/izklop grelnega sistema.

> 📷 *Slika: ESP32-S3-Box-3 in Shelly 2PM Gen3 — strojna oprema projekta*

---

## 📋 Kazalo vsebine

- [Opis projekta](#-opis-projekta)
- [Strojna oprema](#-strojna-oprema)
- [Programska oprema in knjižnice](#-programska-oprema-in-knjižnice)
- [Shema vezave](#-shema-vezave)
- [Konfiguracija Shelly 2PM Gen3](#-konfiguracija-shelly-2pm-gen3)
- [Nastavitev projekta](#-nastavitev-projekta)
- [Konfiguracija parametrov](#-konfiguracija-parametrov)
- [Opis delovanja](#-opis-delovanja)
- [Upravljanje prek zaslona](#-upravljanje-prek-zaslona)
- [Zgradba in nalaganje firmware](#-zgradba-in-nalaganje-firmware)
- [Odpravljanje težav](#-odpravljanje-težav)

---

## 📌 Opis projekta

Projekt implementira **samostojni WiFi termostat** z grafičnim vmesnikom na dotik. Naprava:

- meri temperaturo in vlažnost prek **vgrajenega ali zunanjega senzorja** (npr. SHT30, DHT22),
- prikazuje trenutne vrednosti in nastavitve na **barvnem IPS zaslonu** ESP32-S3-Box-3,
- omogoča nastavljanje **ciljne temperature** neposredno prek zaslona na dotik,
- prek **HTTP REST API klicev** krmili Shelly 2PM Gen3 — vklopi ali izklopi izhod (kanal 1) glede na izmereno temperaturo,
- implementira **histerezno logiko** za preprečitev pogostega preklapljanja,
- opcijsko se integrira z **Home Assistant** prek MQTT ali HTTP.

> 📷 *Slika: Zaslon termostata — prikaz temperature, vlažnosti in nastavljene vrednosti*

---

## 🔧 Strojna oprema

### Zahtevane komponente

| Komponenta | Opis | Opomba |
|---|---|---|
| **ESP32-S3-Box-3** | Razvojni kit z zaslonom, mikrofonom, zvočnikom | Srce naprave |
| **Shelly 2PM Gen3** | WiFi pametno stikalo z merjenjem moči (2 kanala) | Krmili grelec |
| **Temperaturni senzor** | SHT30 (I²C) ali DHT22 | Glej vezavo spodaj |
| **Napajanje ESP32** | USB-C 5V / 2A | Vključeno v Box-3 |
| **Napajanje Shelly** | 110–240 V AC | Pozor: visoka napetost! |

> 📷 *Slika: Shelly 2PM Gen3 — notranjost in priključni sponki*

### Opomba glede senzorja

ESP32-S3-Box-3 **nima vgrajenega temperaturnega senzorja**. Za merjenje temperature in vlažnosti je treba priključiti zunanji senzor:

- **SHT30** (priporočeno): I²C, natančen, enostaven za integracijo
- **DHT22**: enovodna komunikacija, cenovno ugodnejši

---

## 💻 Programska oprema in knjižnice

### Razvojno okolje

- **ESP-IDF** v5.x ali **Arduino IDE** z ESP32 podporo (odvisno od implementacije)
- **PlatformIO** (priporočeno za lažje upravljanje odvisnosti)

### Ključne knjižnice

| Knjižnica | Namen |
|---|---|
| `lvgl` | Grafični vmesnik (GUI) na zaslonu |
| `esp_http_client` | HTTP klici do Shelly API |
| `esp_wifi` | WiFi povezljivost |
| `SHT3x` / `DHT` | Branje senzorja temperature |
| `esp_sntp` | Sinhronizacija ure (opcijsko) |
| `arduino-mqtt` | MQTT integracija (opcijsko) |

---

## 🔌 Shema vezave

### Senzor SHT30 → ESP32-S3-Box-3

```
SHT30           ESP32-S3-Box-3
------          ---------------
VCC  ────────►  3.3V
GND  ────────►  GND
SDA  ────────►  GPIO 8  (I²C Data)
SCL  ────────►  GPIO 18 (I²C Clock)
```

> 📷 *Slika: Fizična vezava senzorja na ESP32-S3-Box-3*

### Shelly 2PM Gen3 — priključitev grelca

```
230V napajanje ──►  L (Shelly vhod)
                    N (Shelly nevtrala)
Shelly izhod O1 ──► Grelec faza
Grelec nevtrala ──► N skupna
```

> ⚠️ **OPOZORILO:** Shelly 2PM Gen3 deluje pri omrežni napetosti 230V AC.  
> Priključevanje sme izvajati **izključno usposobljena oseba**. Pred delom obvezno **izklopite električno napajanje** na varovalniku!

> 📷 *Slika: Shelly 2PM Gen3 — shema priključitve grelca*

---

## ⚙️ Konfiguracija Shelly 2PM Gen3

Preden zaženete termostat, morate Shelly ustrezno nastaviti.

### 1. Začetna nastavitev Shelly

1. Shelly ob prvem zagonu ustvari lastno WiFi točko (`ShellyPlus2PM-XXXXXX`).
2. Povežite se nanjo in odprite `http://192.168.33.1` v brskalniku.
3. Pod **Settings → WiFi** vnesite podatke vašega domačega WiFi omrežja.
4. Po ponovnem zagonu preverite IP naslov Shelly v usmerjevalniku (DHCP lista).

### 2. Nastavitev statičnega IP (priporočeno)

Da ESP32 vedno doseže Shelly na istem naslovu, nastavite statični IP:

- V spletnem vmesniku Shelly: **Settings → WiFi → Static IP**
- Primer: `192.168.1.100`, maska `255.255.255.0`, prehod `192.168.1.1`

### 3. Preveritev HTTP API

V brskalniku ali z `curl` preverite, da API deluje:

```bash
# Stanje naprave
curl http://192.168.1.100/rpc/Shelly.GetStatus

# Vklop izhoda 1 (grelec ON)
curl "http://192.168.1.100/rpc/Switch.Set?id=0&on=true"

# Izklop izhoda 1 (grelec OFF)
curl "http://192.168.1.100/rpc/Switch.Set?id=0&on=false"

# Branje stanja stikala
curl "http://192.168.1.100/rpc/Switch.GetStatus?id=0"
```

> 📷 *Slika: Shelly spletni vmesnik — prikaz stanja in nastavitev*

---

## 🚀 Nastavitev projekta

### 1. Kloniranje repozitorija

```bash
git clone https://github.com/vaš-uporabnik/esp32-termostat.git
cd esp32-termostat
```

### 2. Kopiranje konfiguracijske datoteke

```bash
cp config/config_example.h config/config.h
```

### 3. Urejanje konfiguracije

Odprite `config/config.h` in nastavite parametre (glej naslednje poglavje).

### 4. Nameščanje odvisnosti (PlatformIO)

```bash
pio lib install
```

---

## 🔑 Konfiguracija parametrov

Vse nastavitve so zbrane v datoteki `config/config.h`:

```cpp
// ── WiFi ──────────────────────────────────────────────
#define WIFI_SSID        "VašeOmrežje"
#define WIFI_PASSWORD    "VašeGeslo"

// ── Shelly 2PM Gen3 ───────────────────────────────────
#define SHELLY_IP        "192.168.1.100"   // IP naslov Shelly
#define SHELLY_CHANNEL   0                  // Kanal: 0 = izhod 1, 1 = izhod 2

// ── Temperaturni senzor ───────────────────────────────
#define SENSOR_TYPE      SENSOR_SHT30       // SENSOR_SHT30 ali SENSOR_DHT22
#define SENSOR_SDA_PIN   8
#define SENSOR_SCL_PIN   18

// ── Termostatska logika ───────────────────────────────
#define TEMP_DEFAULT     21.0f              // Privzeta ciljna temperatura (°C)
#define TEMP_MIN         10.0f              // Minimalna nastavitev (°C)
#define TEMP_MAX         30.0f             // Maksimalna nastavitev (°C)
#define TEMP_HYSTERESIS  0.5f              // Histereza (°C) — preprečuje pogosto preklapljanje
#define SENSOR_INTERVAL  30000             // Interval meritev (ms)

// ── MQTT (opcijsko) ───────────────────────────────────
#define MQTT_ENABLED     false
#define MQTT_BROKER      "192.168.1.10"
#define MQTT_PORT        1883
#define MQTT_TOPIC_STATE "termostat/stanje"
#define MQTT_TOPIC_SET   "termostat/nastavi"
```

---

## 🧠 Opis delovanja

### Termostatska logika (histereza)

Naprava deluje po naslednjem principu:

```
Izmerjena temperatura < (Ciljna temperatura − Histereza)
    → Pošlji ukaz Shelly: VKLOPI grelec

Izmerjena temperatura > (Ciljna temperatura + Histereza)
    → Pošlji ukaz Shelly: IZKLOPI grelec

Vmesno območje → Stanje se NE spremeni (preprečevanje treperenja)
```

**Primer** (ciljna temperatura 21°C, histereza 0,5°C):
- Pod 20,5°C → grelec se VKLOPI
- Nad 21,5°C → grelec se IZKLOPI
- Med 20,5°C in 21,5°C → stanje ostane nespremenjeno

### HTTP komunikacija s Shelly

ESP32 pošilja ukaze prek lokalnega WiFi omrežja neposredno na Shelly REST API. Ni potrebe po oblačnih storitvah ali zunanjem posredniku.

```
ESP32-S3-Box-3  ──WiFi──►  Usmerjevalnik  ──LAN──►  Shelly 2PM Gen3
    (termostat)                                          (stikalo)
```

### Stanja sistema

| Stanje | Opis |
|---|---|
| 🔴 `CONNECTING` | Vzpostavljanje WiFi povezave |
| 🟡 `SENSOR_ERROR` | Napaka pri branju senzorja |
| 🟢 `HEATING` | Grelec vklopljen |
| 🔵 `IDLE` | Grelec izklopljen, temperatura dosežena |
| ⚫ `SHELLY_OFFLINE` | Shelly ni dosegljiv |

---

## 📱 Upravljanje prek zaslona

> 📷 *Slika: Zaslonski vmesnik — glavni zaslon termostata*

### Glavni zaslon

- **Zgornji del:** Trenutna temperatura (velika pisava) in vlažnost
- **Sredina:** Nastavljena ciljna temperatura z gumbi `−` in `+`
- **Spodaj:** Ikona stanja grelca (🔥 = vklopljeno, ❄️ = izklopljeno)
- **Statusna vrstica:** WiFi signal, ura, stanje Shelly

### Navigacija

| Gesta / gumb | Dejanje |
|---|---|
| Dotik `+` | Povišaj ciljno temperaturo za 0,5°C |
| Dotik `−` | Znižaj ciljno temperaturo za 0,5°C |
| Dolg pritisk `+` ali `−` | Hitro spreminjanje temperature |
| Dotik ikone nastavitev (⚙️) | Odpri meni nastavitev |
| Gumb na ohišju (levi) | Pojdi nazaj / zbudi zaslon |

### Meni nastavitev

- Ročni/Avtomatski način
- Nastavljanje urnika (opcijsko)
- Informacije o napravi (IP, firmware verzija)
- Ponastavitev na privzete vrednosti

---

## 🏗️ Zgradba in nalaganje firmware

### PlatformIO (priporočeno)

```bash
# Zgradba
pio run

# Nalaganje na ESP32-S3-Box-3
pio run --target upload

# Odpri serijski monitor (115200 baud)
pio device monitor
```

### Arduino IDE

1. Odprite `src/main.cpp` (preimenujte v `main.ino`).
2. V **Board Manager** izberite: `ESP32S3 Dev Module`.
3. Nastavite **Flash Size**: `16MB`, **PSRAM**: `OPI PSRAM`.
4. Naložite s `Ctrl+U`.

### Pričakovani izpis ob zagonu

```
[BOOT] ESP32-S3-Box-3 Termostat v1.0
[WIFI] Povezovanje na 'VašeOmrežje'...
[WIFI] Povezano! IP: 192.168.1.50
[SENSOR] SHT30 inicializiran (I²C: 0x44)
[SHELLY] Dosegljiv na 192.168.1.100 ✓
[THERMOSTAT] Zagotovljeno — ciljna temp: 21.0°C, histereza: 0.5°C
```

---

## 🛠️ Odpravljanje težav

### WiFi se ne poveže

- Preverite SSID in geslo v `config.h`.
- ESP32-S3-Box-3 podpira samo **2,4 GHz** omrežja (ne 5 GHz).
- Preverite, ali je DHCP vklopljen na usmerjevalniku.

### Shelly ni dosegljiv

```bash
# Na računalniku v istem omrežju preverite:
ping 192.168.1.100
curl http://192.168.1.100/rpc/Shelly.GetDeviceInfo
```

- Prepričajte se, da sta ESP32 in Shelly v **istem WiFi omrežju**.
- Preverite, ali usmerjevalnik blokira komunikacijo med napravami (AP isolation).
- Potrdite IP naslov Shelly v DHCP tabeli usmerjevalnika.

### Napaka senzorja temperature

- Preverite ožičenje: VCC → 3.3V (ne 5V!), GND, SDA, SCL.
- Preverite I²C naslov senzorja (SHT30 privzeto: `0x44`).
- V serijskem monitorju preverite I²C scan:

```
[I2C] Iskanje naprav...
[I2C] Najdena naprava na naslovu: 0x44  ✓
```

### Grelec ne reagira

- Preverite ročno z `curl` (glejte poglavje [Konfiguracija Shelly](#-konfiguracija-shelly-2pm-gen3)).
- Preverite, ali je pravi kanal nastavljen (`SHELLY_CHANNEL 0` ali `1`).
- Preverite, ali ima Shelly dovoljenja za lokalni API (v nastavitvah Shelly: **Settings → Local control → Enabled**).

---

## 📡 Opcijska integracija z Home Assistant

Če imate Home Assistant, lahko termostat integrirate prek MQTT:

```yaml
# configuration.yaml
climate:
  - platform: mqtt
    name: "Sobni termostat"
    modes:
      - "off"
      - "heat"
    current_temperature_topic: "termostat/stanje/temperatura"
    temperature_command_topic: "termostat/nastavi/temperatura"
    temperature_state_topic: "termostat/stanje/ciljna_temp"
    mode_command_topic: "termostat/nastavi/nacin"
    mode_state_topic: "termostat/stanje/nacin"
```

> Omogočite MQTT v `config.h` in nastavite `MQTT_BROKER` na IP naslov vašega HA strežnika.

---

## 📄 Licenca

MIT License — prosto za osebno in komercialno uporabo z navedbo avtorja.

---

## 🙏 Zahvala

- [LVGL](https://lvgl.io/) — odlična knjižnica za GUI na vgrajenih sistemih
- [Shelly](https://shelly.cloud/) — za odprt lokalni HTTP API
- [Espressif](https://www.espressif.com/) — za ESP32-S3-Box-3 platformo

---

*Zadnja posodobitev: April 2026*
