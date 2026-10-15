from pathlib import Path
import re

source_path = Path(__file__).resolve().parents[1] / "26C-" / "startmenuwidget.cpp"
source = source_path.read_text(encoding="utf-8")


def source_rect(field):
    match = re.search(
        rf"\b{field}\s*=\s*QRect\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)",
        source,
    )
    if not match:
        raise AssertionError(f"Could not read {field} from {source_path}")
    return tuple(map(int, match.groups()))


START_RECT = source_rect("m_startButtonRect")
HELP_RECT = source_rect("m_helpButtonRect")
EXIT_RECT = source_rect("m_exitButtonRect")

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
    matched_btn = "start" if s else ("help" if h else ("exit" if e else None))
    is_correct = (any_hit == exp_hit) and (matched_btn == exp_btn)

    status = "[PASS]" if is_correct else "[FAIL]"
    if not is_correct:
        all_passed = False

    print(f"{status} ({x:3d}, {y:3d}): {desc} -> hit={any_hit} button={matched_btn}")

assert all_passed, "Some menu geometry tests failed!"
print("\nPASS: Menu source rectangle coordinate checks")
