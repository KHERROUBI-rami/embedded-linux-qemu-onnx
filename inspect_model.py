import onnxruntime as ort

model_path = "remote-sensing/models/model_int8_static.onnx"

session = ort.InferenceSession(
    model_path,
    providers=["CPUExecutionProvider"]
)

print("Inputs:")
for item in session.get_inputs():
    print("  name :", item.name)
    print("  shape:", item.shape)
    print("  type :", item.type)

print("Outputs:")
for item in session.get_outputs():
    print("  name :", item.name)
    print("  shape:", item.shape)
    print("  type :", item.type)
