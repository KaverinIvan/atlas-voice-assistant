============== docs/HARDWARE.md	====================

# Железо и подключение

## Спецификация

| Компонент | Модель | Примечание |
|---|---|---|
| Контроллер | ESP32-S3 DevKit (YD-SP32-23), **N16R8** | 16 MB Flash, 8 MB OPI PSRAM |
| Микрофон | INMP441 (I2S MEMS) | 16-bit, 16 kHz |
| Усилитель | MAX98357A (I2S + Class D) | до 3.2 Вт |
| Динамик | 4–8 Ом, 1–3 Вт | широкополосный |
| Питание | USB 5V / 2A | пиковый ток до 500 мА |

## Распиновка ESP32-S3

### Микрофон INMP441

| INMP441 | ESP32 GPIO | Примечание |
|---|---|---|
| VDD | 3V3 | ⚠️ **3.3 В**, не 5 В |
| GND | GND | |
| L/R | GND | LOW = левый канал |
| SCK | **14** | bit clock |
| WS | **15** | word select |
| SD | **42** | data |

### Усилитель MAX98357A

| MAX98357A | ESP32 GPIO | Примечание |
|---|---|---|
| VIN | **5V** | 2.5–5.5 В, лучше 5 В |
| GND | GND | |
| BCLK | **21** | bit clock |
| LRC | **18** | word select |
| DIN | **41** | data in |
| SD | **38** | shutdown: **HIGH=звук, LOW=тишина** |
| GAIN | — | по умолчанию 9 дБ |
| OUT+ / OUT− | → динамик | не замыкать на GND |

## Схема

```
INMP441                ESP32-S3                 MAX98357A ─→ динамик
┌──────┐               ┌───────────┐            ┌──────┐
│ VDD ─┼──3V3─────────┤            │            │      │
│ GND ─┼──GND─────────┤            │            │      │
│ L/R ─┼──GND         │            │            │      │
│ SCK ─┼──────────────┤ GPIO14     │            │      │
│ WS  ─┼──────────────┤ GPIO15     │            │      │
│ SD  ─┼──────────────┤ GPIO42     │            │      │
└──────┘               │            │            │      │
                       │ GPIO21 ────┼──BCLK─────┤      │
                       │ GPIO18 ────┼──LRC──────┤      │
                       │ GPIO41 ────┼──DIN──────┤      │
                       │ GPIO38 ────┼──SD───────┤      │
                       │ 5V     ────┼──VIN──────┤      │
                       │ GND    ────┼──GND──────┤      │
                       └───────────┘            │      │
                                                │ OUT+ ─┐
                                                │ OUT− ─┼─ динамик
                                                └──────┘
```

## Настройки Arduino IDE

```
Board:              ESP32S3 Dev Module
Flash Size:         16MB (128Mb)
PSRAM:              OPI PSRAM           ← критично! Не QSPI
USB CDC On Boot:    Enabled
Partition Scheme:   16M Flash (3MB APP/9.9MB FATFS)
CPU Frequency:      240MHz (WiFi)
Upload Speed:       921600
```

## Библиотеки Arduino

| Библиотека | Версия | Откуда |
|---|---|---|
| **WebSockets** (Links2004) | ≥2.4.0 | Library Manager |
| **ArduinoJson** | 6.16.1 (или 7.x с правкой) | Library Manager |
| **ESP32 core** | 3.3.10 | Boards Manager |

> ⚠️ ArduinoJson 6.x → `StaticJsonDocument<256>`. В 7.x → `JsonDocument`.

## Частые проблемы железа

| Симптом | Причина | Решение |
|---|---|---|
| `RMS < 50` всегда | L/R висит в воздухе | Посадить L/R на GND |
| Микрофон молчит | Питание 5V | Только 3.3V на VDD |
| Треск в динамике | Питание проседает | Отдельный 5V/2A адаптер |
| Нет звука | `AMP_SD = LOW` | Проверить логику (HIGH=звук) |
| Искажённый голос | Sample rate mismatch | `AMP_SAMPLE_RATE = 22050` |

