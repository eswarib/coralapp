from PIL import Image, ImageDraw
import math
import os

ICON_SIZE = 16
FRAMES = 8
AMPLITUDE = 5
WAVE_LENGTH = 12
BG_COLOR = (13, 71, 161, 255)  # Blue background
WAVE_COLOR = (255, 255, 255, 255)  # white wave

os.makedirs("wave_icons", exist_ok=True)

for frame in range(FRAMES):
    img = Image.new("RGBA", (ICON_SIZE, ICON_SIZE), BG_COLOR)
    draw = ImageDraw.Draw(img)
    for x in range(ICON_SIZE):
        # Sine wave, phase shifted for animation
        y = int(
            ICON_SIZE // 2
            + AMPLITUDE * math.sin(2 * math.pi * (x / WAVE_LENGTH) + 2 * math.pi * frame / FRAMES)
        )
        # Draw a vertical line for a thicker wave
        draw.line([(x, y - 1), (x, y + 1)], fill=WAVE_COLOR, width=1)
    img.save(f"wave_icons/wave_{frame}.png")
print("Wave icons generated in the 'wave_icons' folder.")
