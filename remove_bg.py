from PIL import Image
import numpy as np
import os

def remove_white_background(input_path, output_path, tolerance=20):
    img = Image.open(input_path).convert("RGBA")
    data = np.array(img)

    r, g, b, a = data[:, :, 0], data[:, :, 1], data[:, :, 2], data[:, :, 3]
    white_mask = (r > 255 - tolerance) & (g > 255 - tolerance) & (b > 255 - tolerance)
    data[white_mask, 3] = 0

    result = Image.fromarray(data)
    result.save(output_path)
    print(f"완료: {output_path}")

resource_dir = os.path.join(os.path.dirname(__file__), "resource")

targets = [
    "Generated_Image_green.png",
    "Generated_Image_red.png",
    "Generated_Image_yellow.png",
    "Generated_Image_blue.png",
]

for filename in targets:
    path = os.path.join(resource_dir, filename)
    if os.path.exists(path):
        remove_white_background(path, path)
    else:
        print(f"파일 없음: {filename}")
