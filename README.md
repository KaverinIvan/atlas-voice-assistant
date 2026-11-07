================= README.md =====================

# Atlas Voice Assistant 🎙️

Локальный голосовой ассистент для умного дома на ESP32-S3 + собственный сервер.

- **Wake word** «Атлас» — Vosk (small-ru), работает на CPU, не галлюцинирует
- **ASR** — faster-whisper small + антициклы
- **LLM** — Qwen 27B (primary) → Gemma 12B (fallback)
- **Агент** — Agent_bash для физических действий (файлы, сеть, поиск)
- **TTS** — Piper in-process (ru_RU-denis-medium), стриминг по предложениям
- **ESP32-S3** — I2S микрофон INMP441 + усилитель MAX98357A

## Возможности

| Что | Как |
|---|---|
| Сказать «Атлас, сколько времени» | Wake → ASR → Qwen → голосом |
| Сказать «Атлас, какая погода в Москве» | → Agent_bash (инструменты) |
| Сказать «Атлас, передай вопрос эксперту, как работает TCP» | → Qwen (развёрнуто) |
| Сказать «Атлас, проверь статус docker» | → Agent_bash (auto) |
| Услышать «Слушаю» | Wake подтверждён |
| Услышать «Отключаюсь» | Сказал только «Атлас» и промолчал |

## Быстрый старт

### 1. Сервер (Ubuntu 22.04 / 24.04)

```bash
git clone https://github.com/<your-username>/atlas-voice-assistant.git
cd atlas-voice-assistant

# Установить зависимости
bash scripts/install.sh

# Скачать модели (Vosk, Piper, Whisper, SpeechBrain)
source venv/bin/activate
python scripts/download_models.py

# Скопировать конфиг и отредактировать
cp config.example.py config.py
nano config.py   # ← вписать IP ваших LLM-серверов

# Установить systemd-службу
sudo cp systemd/voice-assistant.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now voice-assistant

# Проверить
curl http://127.0.0.1:8002/api/health
```

### 2. ESP32-S3

Откройте `esp32/voice_assistant_esp32.ino` в Arduino IDE, отредактируйте:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* SERVER_IP = "192.168.1.100";   // ← IP вашего сервера
```

Плата: **ESP32S3 Dev Module**, Flash 16MB, **PSRAM OPI**, USB CDC On Boot = Enabled.

Библиотеки: `WebSockets` (Links2004), `ArduinoJson 6.x`.

Схема подключения — в [docs/HARDWARE.md](docs/HARDWARE.md).

### 3. LLM-серверы

Нужны 2 endpoint'а, совместимых с OpenAI API (`/v1/chat/completions`):

| Роль | Модель | Пример URL |
|---|---|---|
| Primary | Qwen 3.8 27B (или другая) | `http://192.168.1.101:8080/v1` |
| Fallback | Gemma 12B (или другая) | `http://192.168.1.102:8080/v1` |

Оба — `llama.cpp` (llama-server) или любой OpenAI-совместимый шлюз.

### 4. Agent_bash (опционально)

Для физических действий (файлы, сеть, bash) — [Agent_bash GUI](https://github.com/<...>) на `http://127.0.0.1:8001`. Если не нужен — отключите в `config.py`: `AGENT_GUI_URL = None`.

## Документация

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — схема, компоненты, поток данных
- [docs/HARDWARE.md](docs/HARDWARE.md) — распиновка ESP32-S3, микрофон, усилитель
- [docs/SETUP.md](docs/SETUP.md) — полная пошаговая установка
- [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) — проблемы и решения

## Лицензия

**CC BY-NC-SA 4.0** — для личного/некоммерческого использования.  
Коммерческое использование запрещено. Подробности в [LICENSE](LICENSE).

## Благодарности

Проект собрал лучшие практики из множества попыток и переписываний — от Whisper-tiny wake word (галлюцинации) до Vosk (стабильно). Архитектурные решения обоснованы в `docs/ARCHITECTURE.md`.