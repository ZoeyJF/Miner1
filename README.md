# Miner1 - Minero Optimizado para Dispositivos Móviles (ARM64 / Android)

Basado en el código fuente de alta eficiencia de **XMRig**, personalizado y pre-configurado para dispositivos móviles Android con procesadores ARM64 (Snapdragon, MediaTek, Exynos).

## Características principales de esta versión

- **0% Comisiones / 0% Donaciones fijas:** Configurado nativamente en `src/donate.h` (`kDefaultDonateLevel = 0` y `kMinimumDonateLevel = 0`). El 100% de la potencia y recompensas van directamente a tu monedero.
- **Aceleración Hardware ARMv8 Crypto & AES:** Usa instrucciones directas en silicio para maximizar los hashes por segundo.
- **Modo Ligero RandomX (`--randomx-mode=light`):** Consume únicamente ~256 MB de RAM en vez de 2.3 GB, evitando cierres inesperados por parte del kernel de Android (evita el OOM Killer).
- **Afinidad de Núcleos (`--cpu-affinity=0xF0`):** Optimizado para arquitecturas *big.LITTLE*, asignando el trabajo a los núcleos de alto rendimiento.
- **Preconfigurado para MoneroOcean (PPLNS):** Dificultad inicial optimizada (+10000) para envío constante de shares en dispositivos móviles.

---

## Cómo compilar y ejecutar en Termux (Android)

### 1. Requisitos previos
Instala Termux desde F-Droid y ejecuta:

```bash
pkg update -y && pkg upgrade -y
pkg install git cmake clang build-essential libuv openssl -y
```

### 2. Clonar tu repositorio Miner1
```bash
git clone https://github.com/ZoeyJF/Miner1.git
cd Miner1
```

### 3. Compilar
```bash
mkdir build
cd build
cmake .. -DCMAKE_SYSTEM_NAME=Linux -DWITH_HWLOC=OFF -DWITH_CUDA=OFF -DWITH_OPENCL=OFF
make -j$(nproc)
cd ..
```

*Nota: Si estás dentro de un entorno PRoot/Ubuntu en Termux y se queja de `-llog`, ejecuta una sola vez:*
```bash
echo "" | gcc -shared -x c - -o /usr/lib/aarch64-linux-gnu/liblog.so
```

### 4. Iniciar la minería
Ejecuta el script preconfigurado:
```bash
./iniciar_movil.sh
```

---

## Monitoreo en vivo

Puedes consultar las estadísticas y pagos en tiempo real en la web:
- Entra a: `https://moneroocean.stream`
- Pega tu dirección de Monero: `41dCoxb5ewPKLUiTvYQ2ChiaoSxxoQRv9eFruUFRkaNz6uBecPsHxvB22BsB7Cm81K8t79W4PtQiCdPbXUFiGNoRQruL5EL`
