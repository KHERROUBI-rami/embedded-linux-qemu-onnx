# ARM64 Embedded ONNX Runtime Inference

This project implements an ARM64/AArch64 embedded image-classification
pipeline running inside QEMU.

The system receives a multispectral tile through the `/dev/tile` Linux
character device and performs inference using ONNX Runtime and an ONNX model.

## Pipeline

```text
highway.raw
    |
    v
/dev/tile
    |
    v
tile_drv kernel driver
    |
    v
infer userspace application
    |
    v
HWC uint16 -> NCHW float32
    |
    v
Normalization by 10000.0
    |
    v
ONNX Runtime
    |
    v
Classification result
```

## Project components

- `driver/tile/tile_drv.c`: Linux kernel driver exposing `/dev/tile`.
- `driver/tile/Kbuild`: Kernel module build configuration.
- `infer.c`: Userspace inference application.
- `data/highway.raw`: Test input tile.
- `mkimg.sh`: Root filesystem/image build script.
- `run.sh`: QEMU launch script.
- `ai/`: Helper scripts for model and input preparation.

## Input format

The input tile contains:

```text
Width:     64
Height:    64
Channels:  6
Type:      uint16
Layout:    HWC
Size:      64 x 64 x 6 x 2 = 49152 bytes
```

The inference application converts the input from:

```text
HWC uint16
```

to:

```text
NCHW float32
```

with the model input shape:

```text
[1, 6, 64, 64]
```

Values are normalized using:

```text
normalized_value = raw_value / 10000.0
```

## Classes

The model produces the following classes:

```text
AnnualCrop
Forest
HerbaceousVegetation
Highway
Industrial
Pasture
PermanentCrop
Residential
River
SeaLake
```

## Running the system

Inside the ARM64 QEMU environment:

```sh
export ORT_DISABLE_TELEMETRY=1
cat /data/highway.raw > /dev/tile
/usr/bin/infer
```

Example result:

```text
Prediction: Highway (95.14%)
```

The tested input was classified as `Highway` with a probability of `95.14%`.

## ONNX Runtime telemetry warning

ONNX Runtime may display a warning if no CA certificate bundle is available:

```text
No readable CA bundle was found; telemetry HTTPS uploads will be unavailable
```

This warning does not prevent local inference. Telemetry can be disabled with:

```sh
export ORT_DISABLE_TELEMETRY=1
```

## AI assistance disclosure

Parts of the userspace inference code, build commands, debugging process,
and documentation were developed or refined with assistance from an AI coding
assistant.

The project author reviewed, compiled, integrated, and tested the final
implementation. The ARM64 inference pipeline was verified in QEMU using the
`/dev/tile` device and the ONNX model.

The project author remains responsible for reviewing the code, testing,
security, and licensing of the project and its third-party components.

## Third-party components

This project uses third-party components, including Linux, BusyBox, QEMU,
ONNX Runtime, and the ONNX model.

Each third-party component remains subject to its own license. Refer to the
corresponding license and notice files before redistribution.

## Status

The end-to-end ARM64 inference pipeline is operational:

```text
tile input -> /dev/tile -> infer -> ONNX Runtime -> classification
```
