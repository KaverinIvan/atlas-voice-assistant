============== docs/ARCHITECTURE.md =====================

# Архитектура

## Схема

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32-S3 (аудио-сателлит)                    │
│  INMP441 ──I2S RX──┐         ┌──I2S TX── MAX98357A + динамик    │
│                    ▼         ▼                                  │
│              ┌──────────────────────┐                           │
│              │  WebSocket Client    │                           │
│              │  PCM 16kHz / 22050Hz │                           │
│              └──────────┬───────────┘                           │
└─────────────────────────┼───────────────────────────────────────┘
                          │ WiFi
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│              Сервер (<SERVER_IP>:8002)                          │
│                                                                  │
│   PCM → Vosk (wake) → Whisper (ASR) → Router → LLM/Agent        │
│                                                 ↓                │
│                                    Piper (TTS) → PCM → ESP32    │
└─────────────────┬───────────────────────────────────────────────┘
                  │
    ┌─────────────┼──────────────┐
    ▼             ▼              ▼
┌─────────┐  ┌─────────┐   ┌──────────┐
│Primary  │  │Fallback │   │Agent_bash│
│LLM      │  │LLM      │   │(tools)   │
└─────────┘  └─────────┘   └──────────┘
```

## Компоненты

### Сервер

| Файл | Роль |
|---|---|
| `main.py` | FastAPI + WebSocket endpoint, стриминг LLM→TTS |
| `config.py` | Все настройки в одном месте |
| `wake_word.py` | Vosk-детектор wake word «Атлас» |
| `asr.py` | faster-whisper small + post_filter (антициклы) |
| `router.py` | Трёхуровневый роутер: agent / expert / quick |
| `tts.py` | Piper in-process (ONNX) |
| `dialogue.py` | История диалога |
| `agent_bridge.py` | HTTP-клиент Agent_bash API |
| `speaker_verifier.py` | SpeechBrain ECAPA (опционально) |

### Внешние сервисы

| Сервис | Назначение | Протокол |
|---|---|---|
| Primary LLM | Основные ответы | `POST /v1/chat/completions` (OpenAI-совместимый) |
| Fallback LLM | Если primary недоступен | то же |
| Agent_bash | Физические действия | REST (`/api/tasks`) |

## Поток одной реплики

```
1. ESP32 → PCM чанки (40 мс каждый) → Сервер
2. Сервер → Vosk: пробуем распознать «атлас»
3. Vosk сработал → Сервер → ESP32: {"event":"wake"}
4. Сервер → TTS «Слушаю» → ESP32: {"event":"tts_start"}, PCM, {"event":"tts_end"}
5. Сервер → ESP32: {"event":"listening"} → пользователь говорит команду
6. Сервер копит PCM до 3 с тишины → ASR → текст
7. Router → выбор LLM/Agent:
   - agent   → Agent_bash.run_prompt()
   - expert  → Primary LLM (макс. 400 токенов)
   - quick   → Primary LLM (макс. 200 токенов)
8. Стриминг ответа: LLM отдаёт токены → сервер копит до предложения
   → TTS синтезирует → отправляет чанки PCM на ESP32
9. ESP32 проигрывает TTS
10. {"event":"tts_end"}, {"event":"idle"} → цикл замкнут
```

## Ключевые архитектурные решения

### 1. Vosk вместо Whisper для wake word

**Почему:** Whisper (даже tiny) на 2-секундном окне шума галлюцинирует `«аплос аплос аплос...»` и тратит 4 секунды CPU на каждый чек. Vosk small-ru:
- Не галлюцинирует (Kaldi-based, streaming)
- 50–100 мс на 1.5 с аудио
- Работает в фоновом потоке, не блокирует WebSocket

### 2. faster-whisper с beam=1 и temperature=0

**Почему:** `beam_size=5` + `temperature=[0.0, 0.2, ...]` на длинном шуме уходит в цикл `«Сколько времени? Сколько времен? Сколь времени?»`. Greedy (beam=1) + без fallback + `compression_ratio_threshold=2.4` + post_filter по повторяющимся словам — решает.

### 3. Qwen primary, Gemma fallback

Из логов: Qwen 27B на V100 — **33 tok/s**, Gemma 12B на K80 — **6.1 tok/s**. Обратное назначение = в 5 раз медленнее.

### 4. `enable_thinking: false` для Qwen3

Qwen3 по умолчанию тратит токены на `reasoning_content`. Если `max_tokens=120` кончится раньше, чем модель закончит думать → `content=""` → пустой ответ. Параметр `chat_template_kwargs.enable_thinking=false` отключает thinking.

### 5. Стриминг LLM → TTS по предложениям

Пользователь не ждёт полной генерации — первое предложение уходит в TTS, пока LLM генерирует второе. TTFA (time-to-first-audio) падает с 5–10 с до 1–2 с.

### 6. Agent timeout = 15 с + fallback на LLM

Агент с инструментами может думать минуту. Для голоса это неприемлемо. Через 15 с — переключаемся на LLM (без инструментов), которая ответит «не знаю, посмотри сам» за 2 с.

## Протокол WebSocket (ESP32 ↔ Сервер)

**ESP32 → Сервер:** только бинарные фреймы — PCM `int16 LE, 16000 Hz, mono`, чанки по 1280 байт.

**Сервер → ESP32:**
- JSON-события: `{"event":"...","text":...}`
- Бинарные фреймы TTS: `int16 LE, 22050 Hz, mono`

| Событие | Действие ESP32 |
|---|---|
| `listening` | `paused=false` — начинаем слать PCM |
| `wake` | `paused=true` |
| `transcribing` / `thinking` / `agent_thinking` | `paused=true` |
| `tts_start` | `state=PLAYING`, `AMP_SD=HIGH` |
| `tts_end` | `state=IDLE`, `AMP_SD=LOW`, **`paused=true`** (ждём `listening`/`idle`) |
| `idle` | `state=IDLE`, `paused=false` |
| `ping` | не требует действия (heartbeat) |

**Порядок критичен:** `tts_end` не снимает `paused` — это защита от эха. Только `listening` или `idle` возвращают ESP32 в режим прослушивания.