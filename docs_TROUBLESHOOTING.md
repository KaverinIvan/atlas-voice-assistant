================ docs/TROUBLESHOOTING.md =====================

# Troubleshooting

## Сервер

### `ImportError: cannot import name 'XXX' from 'config'`

В `main.py` импортируется переменная, которой нет в `config.py`. Проверьте:
```bash
grep "from config import" -A 30 main.py | head -40
grep "XXX" config.py
```
Добавьте недостающую переменную в `config.py` или уберите из импорта.

### `SyntaxError: keyword argument repeated: vad_filter`

Дублирование параметра в `asr.py`. Проверьте:
```bash
grep -n "vad_filter" asr.py
```
Должно быть одно вхождение.

### `send 1011 (internal error) keepalive ping timeout`

ESP32 отключилась, потому что сервер не слал пинги. Решение — в `main.py` есть `ping_loop()`, отправляющий `{"event":"ping"}` каждые 5 с. Проверьте, что `ping_task = asyncio.create_task(ping_loop(websocket))` запускается.

### `Agent timeout (15s)`

Агент не успел за 15 с. Это нормально — включается fallback на LLM. Если агент всегда таймаутится — проверьте его `curl http://127.0.0.1:8001/api/status`.

### `Post-filter: reject (%r × %d)`

ASR вернул мусор (одно слово повторяется). Это **правильное** поведение — такой текст не идёт в LLM. Если ложных reject'ов много — говорите громче/чётче.

### `VAD filter removed 00:09.180 of audio`

faster-whisper вырезал 9 из 11 секунд — это не речь, а тишина с шумом. Исправлено через `vad_parameters.threshold=0.2` в `asr.py` (уже в финале).

### Сервер зависает при `systemctl restart`

Python-поток в `run_in_executor` не может быть убит извне. В `systemd` — `KillMode=mixed` + `TimeoutStopSec=10`. Если всё равно висит:
```bash
sudo systemctl kill -s SIGKILL voice-assistant
sudo pkill -9 -f "voice_assistant/venv/bin/python"
sudo systemctl reset-failed voice-assistant
sudo systemctl start voice-assistant
```

## Qwen / LLM

### `content=""` (пустой ответ)

Qwen3 — thinking-модель. В запросе должен быть:
```json
{"chat_template_kwargs": {"enable_thinking": false}}
```
Если llama-server не понимает — попробуйте `"enable_thinking": false` на верхнем уровне, или пересоберите chat-template в llama-server.

Проверка вручную:
```bash
curl -s http://<LLM_IP>:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "<model>",
    "messages": [{"role": "user", "content": "Привет"}],
    "max_tokens": 50,
    "chat_template_kwargs": {"enable_thinking": false}
  }' | python3 -m json.tool
```
В `choices[0].message.content` должен быть непустой текст.

### Primary failed at start, fallback to Gemma

Primary LLM недоступен. Проверьте `curl http://<primary>:8080/v1/models`. Если упал — сервер продолжает работать через fallback (медленнее).

## ESP32

### `[WS] Disconnected` каждые 30–40 секунд

Проверьте heartbeat:
```cpp
ws.enableHeartbeat(15000, 3000, 2);
```
И на сервере — `ping_loop()`.

### `RMS=0` или `RMS < 50`

- L/R висит в воздухе → посадить на GND
- Питание микрофона не 3.3V
- Неправильные пины в `#define` (сверить с `docs/HARDWARE.md`)

### Треск при TTS

- Питание 5V/2A (не USB от ноутбука)
- В `.ino` при `tts_start`: `i2s_zero_dma_buffer(I2S_NUM_1);` перед `AMP_SD = HIGH`

### Голос ускорен / звучит не так

Sample rate mismatch. `AMP_SAMPLE_RATE` должен быть **22050** (совпадать с Piper).

### ESP32 не подключается к серверу

```bash
# На сервере
ss -tlnp | grep 8002
curl http://127.0.0.1:8002/api/health

# На ESP32 (Serial Monitor)
ping <server_ip>  # если есть
```
Проверьте `SERVER_IP` в `.ino` и firewall (`ufw allow 8002/tcp`).

## Общие

### Медленный отклик (>5 с)

1. Проверьте primary LLM: `curl http://<primary>:8080/v1/models` — время ответа должно быть <100 мс.
2. Проверьте `tok/s` в логах llama-server. Если <10 — LLM на слабой GPU.
3. `MAX_HISTORY = 4` — история короткая, TTFT низкий.
4. Стриминг работает — первое предложение уходит в TTS до конца генерации.

### Ложные срабатывания wake word

Vosk-версия ловит «атлас» только как отдельное слово. Если всё равно срабатывает на «атас» — добавьте вариант в `wake_word.py`:
```python
WAKE_RE = re.compile(r"\b(атлас|атлаз|атласс|атласа|атласу|атласик)\b", re.I)
```
**Не добавляйте** «ат-т», «атла» и другие обрывки — это вернёт галлюцинации.

### Память

`systemd` ограничивает `MemoryMax=2G`. Если нужно больше (несколько моделей в памяти) — поднять в `voice-assistant.service`. Whisper small ~500 МБ, Vosk ~50 МБ, Piper ~100 МБ, SpeechBrain ~300 МБ. Итого ~1 ГБ.