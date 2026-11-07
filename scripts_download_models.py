============== scripts/download_models.py ==============================

#!/usr/bin/env python3
"""Скачивание всех моделей, нужных ассистенту."""
import os
import sys
import urllib.request
import zipfile
from pathlib import Path

MODELS_DIR = Path("models")
MODELS_DIR.mkdir(exist_ok=True)


def _download(url: str, dest: Path, desc: str):
    if dest.exists() and dest.stat().st_size > 0:
        print(f"  ✓ {desc} уже есть: {dest}")
        return
    print(f"  ↓ {desc} → {dest}")
    urllib.request.urlretrieve(url, dest)


def download_vosk():
    print("[1/4] Vosk small-ru")
    try:
        from huggingface_hub import hf_hub_download
    except ImportError:
        print("  Установите huggingface_hub: pip install huggingface_hub")
        sys.exit(1)

    target = MODELS_DIR / "vosk-model-small-ru-0.22"
    if target.exists() and any(target.iterdir()):
        print(f"  ✓ Vosk уже распакован: {target}")
        return

    zip_path = hf_hub_download(
        repo_id="localstack/vosk-models",
        filename="vosk-model-small-ru-0.22.zip",
        local_dir=str(MODELS_DIR),
    )
    with zipfile.ZipFile(zip_path, "r") as z:
        z.extractall(MODELS_DIR)
    os.unlink(zip_path)
    print(f"  ✓ Распакован в {target}")


def download_piper():
    print("[2/4] Piper ru_RU-denis-medium")
    base = "https://huggingface.co/rhasspy/piper-voices/resolve/main/ru/ru_RU/denis/medium"
    onnx = MODELS_DIR / "ru_RU-denis-medium.onnx"
    json = MODELS_DIR / "ru_RU-denis-medium.onnx.json"
    _download(f"{base}/ru_RU-denis-medium.onnx", onnx, "ONNX")
    _download(f"{base}/ru_RU-denis-medium.onnx.json", json, "JSON")


def download_whisper():
    print("[3/4] faster-whisper small (скачивается автоматически)")
    try:
        from faster_whisper import WhisperModel
        WhisperModel("small", device="cpu", compute_type="int8")
        print("  ✓ Whisper small OK")
    except Exception as e:
        print(f"  ✗ Ошибка: {e}")


def download_speechbrain():
    print("[4/4] SpeechBrain ECAPA (speaker verifier)")
    try:
        from speechbrain.inference.speaker import EncoderClassifier
        EncoderClassifier.from_hparams(
            source="speechbrain/spkrec-ecapa-voxceleb",
            savedir="pretrained_models/spkrec",
        )
        print("  ✓ SpeechBrain OK")
    except Exception as e:
        print(f"  ⚠ SpeechBrain не критичен, пропускаем: {e}")


if __name__ == "__main__":
    print("=== Загрузка моделей Atlas Voice Assistant ===\n")
    download_vosk()
    download_piper()
    download_whisper()
    download_speechbrain()
    print("\n=== Готово ===")