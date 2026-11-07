============== docs/SETUP.md ============

# Полная установка

## 1. Подготовка сервера (Ubuntu 22.04/24.04)

```bash
sudo apt update
sudo apt install -y \
    python3 python3-venv python3-pip \
    ffmpeg libavformat-dev libavcodec-dev libavutil-dev \
    libswscale-dev libswresample-dev libavdevice-dev libavfilter-dev \
    pkg-config build-essential \
    git wget curl unzip
```

> **Зачем FFmpeg dev-libs:** пакет `av` (зависимость `faster-whisper`) собирается из source и требует заголовочные файлы FFmpeg.

## 2. Клонирование и venv

```bash
git clone https://github.com/<your-username>/atlas-voice-assistant.git
cd atlas-voice-assistant

python3 -m venv venv
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt
```

> Если `pip install` падает на `numpy`/`scipy` — установить вручную:
> ```bash
> pip install "numpy>=2.0,<2.8" scipy
> pip install -r requirements.txt
> ```

## 3. Модели

```bash
source venv/bin/activate
python scripts/download_models.py
```

Скачает:
- `models/vosk-model-small-ru-0.22/` (~46 МБ)
- `models/ru_RU-denis-medium.onnx` + `.json` (~60 МБ)
- Whisper small (в `~/.cache/huggingface/`, ~480 МБ)
- SpeechBrain ECAPA (в `pretrained_models/spkrec/`, ~80 МБ)

Проверка:
```bash
python3 -c "from vosk import Model; Model('models/vosk-model-small-ru-0.22'); print('Vosk OK')"
python3 -c "from piper import PiperVoice; PiperVoice.load('models/ru_RU-denis-medium.onnx'); print('Piper OK')"
```

## 4. Конфигурация

```bash
cp config.example.py config.py
nano config.py
```

Обязательно поправить:
- `LLM_PRIMARY_URL` / `LLM_PRIMARY_MODEL`
- `LLM_FALLBACK_URL` / `LLM_FALLBACK_MODEL`
- `AGENT_GUI_URL` (или `None`, если не используете агента)

## 5. LLM-серверы

Нужны 2 OpenAI-совместимых endpoint'а. Проще всего — **llama.cpp (llama-server)**:

```bash
# Пример запуска на отдельной машине (192.168.1.101)
./llama-server \
    -m /path/to/qwen3.8-27b.gguf \
    --host 0.0.0.0 --port 8080 \
    --ctx-size 8192 --n-gpu-layers 99
```

Проверка:
```bash
curl http://192.168.1.101:8080/v1/models
```

> **Для Qwen3:** в запросах должен быть `chat_template_kwargs.enable_thinking=false`. Если ваш llama-server не понимает этот параметр — приложите свой chat-template (см. `docs/TROUBLESHOOTING.md`).

## 6. systemd

```bash
# Заменить <YOUR_USER> на ваше имя
sed "s/<YOUR_USER>/$USER/g" systemd/voice-assistant.service | \
    sudo tee /etc/systemd/system/voice-assistant.service

sudo systemctl daemon-reload
sudo systemctl enable --now voice-assistant
sudo systemctl status voice-assistant
```

Health check:
```bash
curl http://127.0.0.1:8002/api/health
# {"status":"ok","wake_word":"атлас"}
```

## 7. ESP32-S3

1. Arduino IDE → Boards Manager → установить **esp32** (3.x)
2. Library Manager → установить **WebSockets** (Links2004), **ArduinoJson**
3. Открыть `esp32/voice_assistant_esp32.ino`
4. Заменить:
   - `WIFI_SSID`, `WIFI_PASS`
   - `SERVER_IP` → IP сервера ассистента
5. Настройки платы — см. `docs/HARDWARE.md`
6. Upload

Serial Monitor (115200) должен показать:
```
=== Atlas Voice Assistant ESP32-S3 ===
WiFi...
IP: 192.168.1.xxx
I2S RX...
I2S TX...
Ready.
[WS] Connected
```

## 8. Первый запуск

1. Открыть лог сервера: `sudo journalctl -u voice-assistant -f`
2. Сказать «**Атлас**»
3. Ожидается:
   ```
   ✅ WAKE (Vosk): 'атлас'
   🎯 Wake — playing «Слушаю»
   🎤 Listening for command...
   ```
4. Услышать «Слушаю»
5. Сказать команду: «Сколько времени?»
6. Ожидается:
   ```
   ASR: 'сколько времени?' (lang=ru, dur=4.1s)
   Route: quick
   TTS (api): 93696 bytes (2.1s)
   Done. route=quick, len=34
   ```
7. Услышать ответ из динамика

## 9. Firewall (если включён)

```bash
sudo ufw allow 8002/tcp
sudo ufw status
```

## Обновление

```bash
cd atlas-voice-assistant
git pull
source venv/bin/activate
pip install -r requirements.txt
sudo systemctl restart voice-assistant
```