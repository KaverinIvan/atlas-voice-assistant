================ config.example.py ===========================

"""
Voice Assistant — конфигурация.
Скопируйте в config.py и заполните свои значения.
config.py находится в .gitignore и не уходит в репозиторий.
"""

HOST = "0.0.0.0"
PORT = 8002

# ─── ASR ─────────────────────────────────────────────────────────────
WHISPER_MODEL_SIZE = "small"
WHISPER_DEVICE = "cpu"
WHISPER_COMPUTE = "int8"
WHISPER_LANGUAGE = "ru"

# ─── LLM ─────────────────────────────────────────────────────────────
# PRIMARY — основная (быстрая) модель
# Замените IP на адрес вашего llama-server
LLM_PRIMARY_URL = "http://192.168.1.101:8080/v1"
LLM_PRIMARY_MODEL = "qwen3.8-27b"
LLM_PRIMARY_MAX_TOKENS_QUICK = 200
LLM_PRIMARY_MAX_TOKENS_EXPERT = 400
LLM_PRIMARY_TEMPERATURE = 0.7

# FALLBACK — резервная, если PRIMARY недоступна
LLM_FALLBACK_URL = "http://192.168.1.102:8080/v1"
LLM_FALLBACK_MODEL = "/path/to/gemma-12b.gguf"
LLM_FALLBACK_MAX_TOKENS = 150
LLM_FALLBACK_TEMPERATURE = 0.7

# ─── Agent_bash (опционально) ────────────────────────────────────────
# Поставьте None, если не используете агента
AGENT_GUI_URL = "http://127.0.0.1:8001"
AGENT_TIMEOUT = 15

# ─── TTS ─────────────────────────────────────────────────────────────
PIPER_VOICE = "ru_RU-denis-medium"
PIPER_SAMPLE_RATE = 22050

# ─── Диалог ──────────────────────────────────────────────────────────
MAX_HISTORY = 4
VOICE_SYSTEM_PROMPT = (
    "Ты — голосовой помощник. Отвечай максимально кратко: "
    "1-2 предложения. Без markdown, таблиц, списков и кода. "
    "Только разговорная речь на русском."
)

# ─── Wake word ───────────────────────────────────────────────────────
WAKE_WORD = "атлас"
WAKE_WINDOW_SEC = 1.5
WAKE_COOLDOWN_SEC = 8.0
VOSK_MODEL_PATH = "models/vosk-model-small-ru-0.22"

# ─── Команда ─────────────────────────────────────────────────────────
COMMAND_SILENCE_SEC = 3.0
COMMAND_MAX_SEC = 20.0
COMMAND_MIN_BYTES = 3200
COMMAND_MIN_RMS = 25

# ─── TTS chunks ──────────────────────────────────────────────────────
TTS_CHUNK_SIZE = 2048
TTS_CHUNK_DELAY_MS = 95

# ─── Голосовые фразы ─────────────────────────────────────────────────
WAKE_RESPONSE = "Слушаю."
TIMEOUT_RESPONSE = "Отключаюсь."