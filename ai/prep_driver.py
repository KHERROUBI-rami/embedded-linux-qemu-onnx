import sys

import numpy as np
import tifffile


if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} INPUT.tif OUTPUT.raw")
    sys.exit(1)


input_file = sys.argv[1]
output_file = sys.argv[2]

image = tifffile.imread(input_file)

print("Input shape:", image.shape)
print("Input dtype:", image.dtype)

if image.shape != (64, 64, 6):
    raise ValueError(f"Expected shape (64, 64, 6), got {image.shape}")

# القيم في ملفاتك مثل 345 إلى 7481.
# نحولها إلى uint16 من دون القسمة على 10000.
tile = np.clip(np.rint(image), 0, 65535).astype(np.uint16)

tile.tofile(output_file)

print("Output:", output_file)
print("Output size:", tile.nbytes, "bytes")
