from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "Ui"
EXAMPLES = ROOT / "examples"
COLOR = UI / "ColorPicker"

TEXT_EXTS = {".h", ".cpp", ".upp", ".md", ".tpp", ".txt"}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def expand_inc(path: Path, stack=None) -> str:
    stack = [] if stack is None else stack
    path = path.resolve()
    if path in stack:
        raise RuntimeError(f"recursive include while expanding {path}")
    stack = stack + [path]
    out = []
    for line in read(path).splitlines(keepends=True):
        match = re.match(r'^\s*#include\s+"([^"]+\.inc)"\s*$', line.rstrip("\r\n"))
        if match:
            child = path.parent / match.group(1)
            if not child.exists():
                raise RuntimeError(f"missing include {child} referenced by {path}")
            expanded = expand_inc(child, stack)
            out.append(expanded)
            if expanded and not expanded.endswith("\n"):
                out.append("\n")
        else:
            out.append(line)
    return "".join(out)


def clean_color_picker() -> None:
    COLOR.mkdir(parents=True, exist_ok=True)

    picker_cpp = expand_inc(UI / "UiColorPicker.cpp")
    picker_cpp = picker_cpp.replace('#include "UiColorPicker.h"', '#include <Ui/ColorPicker/UiColorPicker.h>')
    picker_cpp = picker_cpp.replace('#include "UiColorPickerPaletteLab.h"', '#include <Ui/ColorPicker/UiColorPickerPaletteLab.h>')

    lab_cpp = expand_inc(UI / "UiColorPickerPaletteLab.cpp")
    lab_cpp = lab_cpp.replace('#include "UiColorPickerPaletteLab.h"', '#include <Ui/ColorPicker/UiColorPickerPaletteLab.h>')

    picker_h = read(UI / "UiColorPicker.h")
    lab_h = read(UI / "UiColorPickerPaletteLab.h")
    lab_h = lab_h.replace("#include <Ui/UiColorPicker.h>", "#include <Ui/ColorPicker/UiColorPicker.h>")

    write(COLOR / "UiColorPicker.h", picker_h)
    write(COLOR / "UiColorPicker.cpp", picker_cpp)
    write(COLOR / "UiColorPickerPaletteLab.h", lab_h)
    write(COLOR / "UiColorPickerPaletteLab.cpp", lab_cpp)

    for path in UI.glob("UiColorPicker*.inc"):
        path.unlink()
    for name in ("UiColorPicker.h", "UiColorPicker.cpp", "UiColorPickerPaletteLab.h", "UiColorPickerPaletteLab.cpp"):
        path = UI / name
        if path.exists():
            path.unlink()

    # Active source/documentation references must use the new component path.
    for path in ROOT.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in TEXT_EXTS:
            continue
        text = read(path)
        new = text.replace("<Ui/UiColorPicker.h>", "<Ui/ColorPicker/UiColorPicker.h>")
        new = new.replace("<Ui/UiColorPickerPaletteLab.h>", "<Ui/ColorPicker/UiColorPickerPaletteLab.h>")
        if new != text:
            write(path, new)


def strip_header_for_bundle(text: str) -> str:
    lines = []
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith("#ifndef _Ui_UiComposite") or stripped.startswith("#define _Ui_UiComposite"):
            continue
        if stripped == "#endif":
            continue
        if "<Ui/Composites/" in line:
            continue
        lines.append(line)
    return "".join(lines)


def strip_cpp_for_bundle(text: str) -> str:
    return "".join(line for line in text.splitlines(keepends=True)
                   if "#include <Ui/Composites/" not in line)


def build_demo_rows() -> None:
    composites = UI / "Composites"
    if not composites.exists():
        return

    order = ["Slider", "Toggle", "Color", "Dropdown", "Label", "Edit"]
    header = [
        "#ifndef _examples_DemoPropertyRows_h_\n",
        "#define _examples_DemoPropertyRows_h_\n\n",
        "// Transitional demo-only property rows.\n",
        "// These are intentionally outside the Ui package: production code should use\n",
        "// primitive Ui controls, UiSliderEdit, UiColorMatrix, and PropertyEditor.\n\n",
    ]
    for name in order:
        header.append(strip_header_for_bundle(read(composites / f"UiComposite{name}.h")))
        header.append("\n")
    for name in order:
        header.append(strip_cpp_for_bundle(read(composites / f"UiComposite{name}.cpp")))
        header.append("\n")
    header.append("#endif\n")
    text = "".join(header)

    replacements = {
        "UiCompositeLayoutMode": "DemoRowLayoutMode",
        "UICOMPOSITE_INLINE": "DEMO_ROW_INLINE",
        "UICOMPOSITE_STACKED": "DEMO_ROW_STACKED",
        "UiCompositeSlider": "DemoSliderRow",
        "UiCompositeToggle": "DemoToggleRow",
        "UiCompositeColor": "DemoColorRow",
        "UiCompositeDropdown": "DemoDropdownRow",
        "UiCompositeLabel": "DemoLabelRow",
        "UiCompositeEdit": "DemoEditRow",
    }
    for old, new in replacements.items():
        text = text.replace(old, new)
    text = text.replace("<Ui/UiColorPicker.h>", "<Ui/ColorPicker/UiColorPicker.h>")
    write(EXAMPLES / "DemoPropertyRows.h", text)

    support = EXAMPLES / "BuilderDemoSupport.h"
    support_text = read(support)
    marker = "#include <Ui/Ui.h>\n"
    if '#include "DemoPropertyRows.h"' not in support_text:
        support_text = support_text.replace(marker, marker + '#include "DemoPropertyRows.h"\n')
    write(support, support_text)

    for path in EXAMPLES.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in {".h", ".cpp"}:
            continue
        content = read(path)
        new = content
        for old, replacement in replacements.items():
            new = new.replace(old, replacement)
        if new != content:
            write(path, new)

    shutil.rmtree(composites)


def update_ui_package() -> None:
    upp = UI / "Ui.upp"
    lines = read(upp).splitlines(keepends=True)
    out = []
    inserted_color = False
    for line in lines:
        stripped = line.strip().rstrip(",")
        if stripped.startswith("Composites/"):
            continue
        if stripped.startswith("UiColorPicker"):
            continue
        if stripped == "src.tpp" and not inserted_color:
            out.extend([
                "\tColorPicker/UiColorPicker.h,\n",
                "\tColorPicker/UiColorPicker.cpp,\n",
                "\tColorPicker/UiColorPickerPaletteLab.h,\n",
                "\tColorPicker/UiColorPickerPaletteLab.cpp,\n",
            ])
            inserted_color = True
        out.append(line)
    if not inserted_color:
        raise RuntimeError("could not locate src.tpp insertion point in Ui.upp")
    write(upp, "".join(out))

    umbrella = UI / "Ui.h"
    text = read(umbrella)
    text = text.replace("    - 2026-05: composite property-row controls moved under Ui/Composites while\n      remaining available through this umbrella include.\n", "")
    text = text.replace("#include <Ui/UiColorPicker.h>", "#include <Ui/ColorPicker/UiColorPicker.h>")
    text = "\n".join(line for line in text.splitlines()
                       if "#include <Ui/Composites/" not in line) + "\n"
    write(umbrella, text)


def clean_docs() -> None:
    guide = ROOT / "docs" / "01_UI_CONTROLS_GUIDE.md"
    if guide.exists():
        text = read(guide)
        # Remove stale quick-reference bullets/paragraphs that still advertise
        # the retired production UiComposite family. The demo-only compatibility
        # rows are deliberately not public controls and are not documented here.
        lines = []
        skipping = False
        for line in text.splitlines(keepends=True):
            if line.startswith("### UiComposite") or line.startswith("## UiComposite"):
                skipping = True
                continue
            if skipping and (line.startswith("### ") or line.startswith("## ") or line.startswith("---")):
                skipping = False
            if not skipping and "UiComposite" not in line:
                lines.append(line)
        text = "".join(lines)
        note = ("\n> **Retired transitional API:** the former `UiComposite*` property-row family "
                "has been removed from the production library. Use primitive `Ui` controls, "
                "`UiSliderEdit`, `UiColorMatrix`, and `PropertyEditor` composition instead.\n")
        if "Retired transitional API" not in text:
            insert = text.find("\n---\n")
            if insert >= 0:
                text = text[:insert] + note + text[insert:]
            else:
                text += note
        write(guide, text)


def verify_structure() -> None:
    if (UI / "Composites").exists():
        raise RuntimeError("Ui/Composites still exists")
    incs = sorted(UI.rglob("*.inc"))
    if incs:
        raise RuntimeError("Ui still contains .inc files: " + ", ".join(str(p.relative_to(ROOT)) for p in incs))
    required = [
        COLOR / "UiColorPicker.h",
        COLOR / "UiColorPicker.cpp",
        COLOR / "UiColorPickerPaletteLab.h",
        COLOR / "UiColorPickerPaletteLab.cpp",
        UI / "UiColorMatrix.h",
        UI / "UiColorMatrix.cpp",
    ]
    for path in required:
        if not path.exists():
            raise RuntimeError(f"missing {path.relative_to(ROOT)}")
    if ".inc" in read(COLOR / "UiColorPicker.cpp") or ".inc" in read(COLOR / "UiColorPickerPaletteLab.cpp"):
        raise RuntimeError("expanded ColorPicker implementation still references .inc")
    if "Composites/" in read(UI / "Ui.upp") or "<Ui/Composites/" in read(UI / "Ui.h"):
        raise RuntimeError("production package still references Ui/Composites")


def main() -> None:
    clean_color_picker()
    build_demo_rows()
    update_ui_package()
    clean_docs()
    verify_structure()
    print("One-shot Ui library source-structure cleanup complete")


if __name__ == "__main__":
    main()
