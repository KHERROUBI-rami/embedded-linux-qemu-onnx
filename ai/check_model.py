import glob
import re
import sys

import numpy as np
import onnxruntime as ort
import tifffile


CLASSES = [
    "AnnualCrop",
    "Forest",
    "HerbaceousVegetation",
    "Highway",
    "Industrial",
    "Pasture",
    "PermanentCrop",
    "Residential",
    "River",
    "SeaLake",
]


if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} MODEL.onnx")
    sys.exit(1)


model_path = sys.argv[1]

session = ort.InferenceSession(
    model_path,
    providers=["CPUExecutionProvider"],
)

input_name = session.get_inputs()[0].name
output_name = session.get_outputs()[0].name

print("Input:", input_name)
print("Output:", output_name)

files = sorted(glob.glob("/home/navi/arm-lab/test/*.tif"))

if not files:
    print("No TIFF files found.")
    sys.exit(1)

correct = 0

for filename in files:
    image = tifffile.imread(filename).astype(np.float32)

    # صورك الحالية تحتوي مسبقاً على 6 نطاقات:
    # (64, 64, 6)
    #
    # هذا الشرط يدعم أيضاً صورة كاملة ذات 13 نطاقاً.
    if image.shape == (64, 64, 13):
        image = image[:, :, [1, 2, 3, 7, 10, 11]]

    if image.shape != (64, 64, 6):
        print(f"Unexpected shape: {image.shape} in {filename}")
        continue

    # تحويل الشكل من:
    # (64, 64, 6)
    # إلى:
    # (1, 6, 64, 64)
    tensor = image.transpose(2, 0, 1)[None]

    # نفس التطبيع المستخدم أثناء تدريب النموذج
    tensor = (tensor / 10000.0).astype(np.float32)

    result = session.run(
        [output_name],
        {input_name: tensor},
    )[0][0]

    predicted = CLASSES[int(np.argmax(result))]

    match = re.search(r"class_(.+?)__idx", filename)
    expected = match.group(1) if match else "Unknown"

    is_correct = predicted == expected

    if is_correct:
        correct += 1

    status = "OK" if is_correct else "WRONG"

    print(f"{expected:24s} -> {predicted:24s} {status}")

print()
print(f"Result: {correct} / {len(files)} correct")
