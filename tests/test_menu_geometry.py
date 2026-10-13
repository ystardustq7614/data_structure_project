import sys
from PIL import Image
import numpy as np

# Load menu background image
img_path = '26C-/images/menu_bg.jpg'
try:
    img = Image.open(img_path)
    arr = np.array(img)
    print(f"[OK] Successfully loaded {img_path}, dimensions: {img.size}")
except Exception as e:
    print(f"[FAIL] Could not load {img_path}: {e}")
    sys.exit(1)

# Calibrated QRect bounding boxes from startmenuwidget.cpp
# QRect(x, y, w, h)
START_RECT = (218, 345, 303, 83) # X[218, 520], Y[345, 427]
HELP_RECT  = (239, 446, 261, 63) # X[239, 499], Y[446, 508]
EXIT_RECT  = (239, 526, 261, 63) # X[239, 499], Y[526, 588]

def in_rect(x, y, r):
    rx, ry, rw, rh = r
    return rx <= x <= rx + rw - 1 and ry <= y <= ry + rh - 1

def check_hit(x, y):
    s = in_rect(x, y, START_RECT)
    h = in_rect(x, y, HELP_RECT)
    e = in_rect(x, y, EXIT_RECT)
    return s, h, e, (s or h or e)

# Test cases including all points specified by Codex in Section 10.3
test_cases = [
    # (x, y, expected_hit, expected_button, description)
    (300, 335, False, None,    "Codex check: Start button top blank (grass)"),
    (300, 426, True,  "start", "Codex check: Start button bottom visible edge (wood)"),
    (300, 440, False, None,    "Codex check: Help button top blank (grass)"),
    (230, 560, False, None,    "Codex check: Exit button left blank (grass)"),
    (369, 386, True,  "start", "Start button center"),
    (225, 386, True,  "start", "Start button left inner"),
    (515, 386, True,  "start", "Start button right inner"),
    (369, 477, True,  "help",  "Help button center"),
    (245, 477, True,  "help",  "Help button left inner"),
    (490, 477, True,  "help",  "Help button right inner"),
    (369, 557, True,  "exit",  "Exit button center"),
    (245, 557, True,  "exit",  "Exit button left inner"),
    (490, 557, True,  "exit",  "Exit button right inner"),
    (350, 436, False, None,    "Inter-button gap: between Start and Help"),
    (350, 517, False, None,    "Inter-button gap: between Help and Exit"),
]

all_passed = True
print("\n=== Running Menu Button Hitbox & Geometry Tests ===")
for x, y, exp_hit, exp_btn, desc in test_cases:
    s, h, e, any_hit = check_hit(x, y)
    rgb = arr[y, x].tolist()

    matched_btn = "start" if s else ("help" if h else ("exit" if e else None))
    is_correct = (any_hit == exp_hit) and (matched_btn == exp_btn)

    status = "[PASS]" if is_correct else "[FAIL]"
    if not is_correct:
        all_passed = False

    print(f"{status} ({x:3d}, {y:3d}) RGB={rgb}: {desc} -> hit={any_hit} button={matched_btn}")

assert all_passed, "Some menu geometry tests failed!"
print("\nALL MENU GEOMETRY TESTS PASSED PERFECTLY!")
