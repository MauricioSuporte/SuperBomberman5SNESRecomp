"""Writes the launcher icons (res/mipmap-*/ic_launcher.png) from the project's
own assets/app.ico, so no extra image is tracked. Needs Pillow."""
import pathlib
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parent
ico = Image.open(ROOT.parent / "assets" / "app.ico")
ico.size = max(ico.ico.sizes())
img = ico.convert("RGBA")
for name, size in {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}.items():
    out = ROOT / "app" / "src" / "main" / "res" / f"mipmap-{name}" / "ic_launcher.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    img.resize((size, size), Image.LANCZOS).save(out)
print("make_icons: launcher icons written")
