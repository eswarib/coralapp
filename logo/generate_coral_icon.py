from PIL import Image, ImageDraw
import math
import os

ICON_SIZE = 32
BG_COLOR = (0, 0, 0, 0)  # Transparent
WAVE_COLOR = (255, 255, 255, 255)  # White
WAVE_WIDTH = 3

img = Image.new("RGBA", (ICON_SIZE, ICON_SIZE), BG_COLOR)
draw = ImageDraw.Draw(img)

# Draw three identical wavy lines
for i in range(3):
    points = []
    y_offset = 8 + i * 8
    for x in range(ICON_SIZE):
        y = int(y_offset + 4 * math.sin(2 * math.pi * (x / ICON_SIZE)))
        points.append((x, y))
    draw.line(points, fill=WAVE_COLOR, width=WAVE_WIDTH)

#os.makedirs("logo", exist_ok=True)
img.save("coral.png")
print("Saved coral.png")
