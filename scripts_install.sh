============= scripts/install.sh ===================

#!/usr/bin/env bash
# Установка зависимостей сервера Atlas Voice Assistant
set -e

echo "==> Установка системных пакетов (FFmpeg dev-libs, build tools)"
sudo apt update
sudo apt install -y \
    python3 python3-venv python3-pip \
    ffmpeg libavformat-dev libavcodec-dev libavutil-dev \
    libswscale-dev libswresample-dev libavdevice-dev libavfilter-dev \
    pkg-config build-essential \
    git wget curl unzip

echo "==> Создание venv"
python3 -m venv venv
source venv/bin/activate

echo "==> Обновление pip"
pip install --upgrade pip

echo "==> Установка Python-зависимостей"
pip install -r requirements.txt

echo "==> Готово."
echo ""
echo "Дальше:"
echo "  1. source venv/bin/activate"
echo "  2. python scripts/download_models.py"
echo "  3. cp config.example.py config.py && nano config.py"
echo "  4. sudo cp systemd/voice-assistant.service /etc/systemd/system/"
echo "  5. sudo systemctl daemon-reload && sudo systemctl enable --now voice-assistant"

##################### Не забудьте chmod +x scripts/install.sh ############################