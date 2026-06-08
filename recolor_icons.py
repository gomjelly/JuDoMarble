from PIL import Image
import numpy as np

def recolor_and_remove_bg(input_path, output_path, target_color_rgb, tolerance=60, white_tol=30):
    img = Image.open(input_path).convert("RGBA")
    data = np.array(img, dtype=np.float32)

    r, g, b = data[:, :, 0], data[:, :, 1], data[:, :, 2]

    # 흰 배경 투명화
    white_mask = (r > 255 - white_tol) & (g > 255 - white_tol) & (b > 255 - white_tol)

    # 아이콘 픽셀 감지 (어두운 청록 계열 + 검정 윤곽선)
    # 흰 배경이 아닌 모든 픽셀을 아이콘으로 간주
    icon_mask = ~white_mask

    # 아이콘 색을 target_color로 교체 (원본 밝기 비율 유지)
    tr, tg, tb = target_color_rgb

    # 원본 픽셀 밝기(0~1)
    brightness = np.clip((r + g + b) / (3.0 * 255.0), 0, 1)

    result = data.copy().astype(np.uint8)

    # 아이콘 영역: 타겟 색 * 밝기
    result[icon_mask, 0] = np.clip(tr * brightness[icon_mask] * 1.5, 0, 255).astype(np.uint8)
    result[icon_mask, 1] = np.clip(tg * brightness[icon_mask] * 1.5, 0, 255).astype(np.uint8)
    result[icon_mask, 2] = np.clip(tb * brightness[icon_mask] * 1.5, 0, 255).astype(np.uint8)
    result[icon_mask, 3] = 255

    # 흰 배경 투명화
    result[white_mask, 3] = 0

    Image.fromarray(result).save(output_path)
    print(f"완료: {output_path}")

src = "resource/Generated_Image2.png"

configs = [
    ("resource/Generated_Image_red.png",    (220, 50,  50)),
    ("resource/Generated_Image_yellow.png", (220, 180, 30)),
    ("resource/Generated_Image_green.png",  (50,  180, 80)),
    ("resource/Generated_Image_blue.png",   (50,  100, 220)),
]

for out_path, color in configs:
    recolor_and_remove_bg(src, out_path, color)
