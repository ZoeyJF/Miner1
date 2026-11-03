#!/bin/bash
# Script de inicio optimizado para dispositivos móviles (Android / Termux)
# Moneda: Monero (XMR)
# Pool: MoneroOcean (PPLNS)

WALLET="41dCoxb5ewPKLUiTvYQ2ChiaoSxxoQRv9eFruUFRkaNz6uBecPsHxvB22BsB7Cm81K8t79W4PtQiCdPbXUFiGNoRQruL5EL"
POOL="gulf.moneroocean.stream:10128"
PASS="movil1~rx/0+10000"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$SCRIPT_DIR/build/xmrig"

if [ ! -f "$BIN" ]; then
    echo "[!] No se encontró el binario compilado en $BIN"
    echo "[*] Compilando XMRig para móvil..."
    mkdir -p "$SCRIPT_DIR/build"
    cd "$SCRIPT_DIR/build"
    cmake .. -DCMAKE_SYSTEM_NAME=Linux -DWITH_HWLOC=OFF -DWITH_CUDA=OFF -DWITH_OPENCL=OFF
    make -j$(nproc)
    cd "$SCRIPT_DIR"
fi

echo "[*] Iniciando Miner1 optimizado para móvil..."
"$BIN" \
  -o "$POOL" \
  -u "$WALLET" \
  -p "$PASS" \
  -a rx/0 \
  --randomx-mode=light \
  --threads=4 \
  --cpu-affinity=0xF0 \
  --donate-level=0
