# Настройка окружения

## Установка зависимостей

sudo apt update
sudo apt install -y build-essential cmake python3-pip
sudo apt install -y protobuf-compiler libprotobuf-dev
sudo apt install -y libgrpc++-dev libgrpc-dev
sudo apt install -y libboost-system-dev

pip3 install pytest grpcio grpcio-tools --break-system-packages

## Сборка и тесты
./build.sh
./test.sh
