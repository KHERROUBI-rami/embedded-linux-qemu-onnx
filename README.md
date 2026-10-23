# ARM64 Embedded ONNX Runtime Inference

This project implements an ARM64/AArch64 embedded image-classification
pipeline running inside QEMU.

The system receives a multispectral tile through the `/dev/tile` Linux
character device and performs image classification using ONNX Runtime and
an ONNX model.

## Pipeline

```text
data/highway.raw
        |
        v
    /dev/tile
        |
        v
tile_drv Linux kernel driver
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

- `driver/tile/tile_drv.c`
  Linux kernel driver exposing the `/dev/tile` character device.

- `driver/tile/Kbuild`
  Kernel module build configuration.

- `infer.c`
  Userspace application that reads the tile and runs ONNX Runtime inference.

- `data/highway.raw`
  Test input tile used by the inference pipeline.

- `mkimg.sh`
  Script used to build the root filesystem or QEMU image.

- `run.sh`
  Script used to start the ARM64 QEMU environment.

- `ai/`
  Helper scripts for model inspection, data preparation, and preprocessing.

- `preprocess_test.c`
  Test program for validating input preprocessing.

- `tile_read_test.c`
  Test program for reading data from `/dev/tile`.

## Input format

The input tile has the following format:

```text
Width:     64
Height:    64
Channels:  6
Type:      uint16
Layout:    HWC
Size:      64 x 64 x 6 x 2 = 49152 bytes
```

The SHA-256 checksum of the tested input file is:

```text
1ff270e57b3e40c6fcd75b1ec48a22a161315fc97f393b085165e953e476325d
```

The input is originally stored as:

```text
HWC uint16
```

The `infer` application converts it to:

```text
NCHW float32
```

The model input shape is:

```text
[1, 6, 64, 64]
```

Each input value is normalized using:

```text
normalized_value = raw_value / 10000.0
```

## Model classes

The model produces the following ten classes:

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

## Building the kernel module

The driver is built as a Linux kernel module for ARM64/AArch64.

The project uses the following Linux kernel version during development:

```text
Linux 6.6
```

The compiled kernel module is generated during the build process and is not
committed to Git.

Generated files such as the following are intentionally ignored:

```text
*.ko
*.o
*.cmd
Module.symvers
modules.order
```

## Building the userspace inference application

The inference application requires:

- An AArch64 cross-compiler
- ONNX Runtime for AArch64
- Linux system libraries
- The ONNX model file

An example cross-compilation command is:

```bash
aarch64-linux-gnu-gcc -O2 \
  -Ionnxruntime/include \
  -o infer infer.c \
  -Lonnxruntime/lib \
  -Wl,--no-as-needed \
  -Wl,-rpath,/usr/lib \
  -l:libonnxruntime.so.1.29.0 \
  -lm -ldl -lrt -lpthread
```

The resulting executable should be an ARM64/AArch64 binary.

Check the binary architecture with:

```bash
file infer
```

## Running inside QEMU

After building the kernel, driver, root filesystem, ONNX Runtime libraries,
model, and userspace application, start QEMU using:

```bash
./run.sh
```

Inside the ARM64 QEMU environment, run:

```sh
export ORT_DISABLE_TELEMETRY=1
cat /data/highway.raw > /dev/tile
/usr/bin/infer
```

The expected result is:

```text
Prediction: Highway (95.14%)
```

The tested sample was classified as:

```text
Highway
```

with a probability of:

```text
95.14%
```

## ONNX Runtime telemetry

ONNX Runtime may display the following warning when no CA certificate
bundle is available:

```text
No readable CA bundle was found; telemetry HTTPS uploads will be unavailable
```

This warning does not prevent local inference.

Telemetry can be disabled for the current shell with:

```sh
export ORT_DISABLE_TELEMETRY=1
```

## SSH access with Dropbear

The ARM64 root filesystem can include Dropbear as a lightweight SSH server.

When Dropbear is included in the generated root filesystem and QEMU is
configured with host port forwarding, the system can be accessed from the
host with:

```bash
ssh -p 2222 root@127.0.0.1
```

The exact SSH command depends on the port forwarding options configured in
`run.sh`.

The project uses the following Dropbear source version:

```text
Dropbear 2024.85
```

Dropbear source archives, generated binaries, and extracted source trees are
not committed to Git. They are downloaded or built as part of the image
creation process.

## Build dependencies

The development environment uses the following components:

```text
Linux kernel:       6.6
BusyBox:            1.36.1
Dropbear:           2024.85
ONNX Runtime:       1.29.0
Architecture:       ARM64/AArch64
Emulator:           QEMU
```

The exact build commands and URLs may depend on the host distribution and
cross-compilation environment.

## Generated files

The following files are intentionally not committed to Git:

- Generated root filesystem files
- `rootfs.img`
- Kernel binaries
- Compiled kernel modules
- Object files
- Downloaded third-party archives
- Extracted Linux, BusyBox, and Dropbear source trees
- ONNX Runtime binaries
- Generated temporary files

These files can be recreated by the build scripts or downloaded from their
respective official sources.

## Third-party components

This project uses third-party components, including:

- Linux
- BusyBox
- Dropbear
- QEMU
- ONNX Runtime
- The ONNX model
- Remote-sensing test data

Each third-party component remains subject to its own license.

The MIT license in this repository applies only to the original project code
unless otherwise stated. It does not replace or override the licenses of
third-party components.

## AI assistance disclosure

Parts of the userspace inference code, build commands, debugging process,
and documentation were developed or refined with assistance from an AI
coding assistant.

The project author reviewed, compiled, integrated, and tested the final
implementation. The ARM64 inference pipeline was verified in QEMU using
the `/dev/tile` device, ONNX Runtime, and the ONNX model.

The project author remains responsible for reviewing the code, testing,
security, runtime behavior, and licensing requirements.

## Current status

The end-to-end ARM64 inference pipeline is operational:

```text
raw tile
  -> /dev/tile
  -> tile_drv
  -> infer
  -> ONNX Runtime
  -> image classification
```

The tested sample was classified as:

```text
Highway
```

with a probability of:

```text
95.14%
```
