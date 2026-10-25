"""Encode real framebuffer captures as bounded PNG/GIF assets; verify committed media separately.

Requires Pillow only for this optional documentation workflow. PNG preserves framebuffer RGB;
GIF uses one shared 256-color palette and no dithering to avoid animation palette flicker.
No desktop capture, generated illustration, overlays, or physics interpolation is performed.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from PIL import Image


# scene, filename, frame count, pixel dimensions; GIF playback is 20 fps, not simulation time.
ASSETS = (
    ("workspace", "workspace.png", 1, (1440, 900)),
    ("dipole", "electric-dipole.png", 1, (960, 600)),
    ("magnitude", "electric-plane.png", 1, (1440, 900)),
    ("gravity", "earth-moon.gif", 100, (960, 600)),
    ("gyro", "magnetic-gyro.gif", 100, (960, 600)),
)


def read_frames(directory: Path, count: int, size: tuple[int, int]) -> list[Image.Image]:
    """Read a complete, ordered PPM sequence; reject missing, extra, or wrongly sized frames."""
    expected = [directory / f"frame-{index:03d}.ppm" for index in range(count)]
    if sorted(directory.glob("frame-*.ppm")) != expected:
        raise ValueError(f"{directory}: expected exactly {count} consecutive PPM frames")
    frames = []
    for path in expected:
        with Image.open(path) as image:
            if image.size != size or image.format != "PPM":
                raise ValueError(f"{path}: expected {size} RGB framebuffer image")
            frames.append(image.convert("RGB"))
    return frames


def encode_gif(frames: list[Image.Image], destination: Path) -> None:
    """Use representative real frames for a shared palette; retain all 100 fixed-step frames."""
    width, height = frames[0].size
    atlas = Image.new("RGB", (width * 4, height * 4))
    for index in range(16):
        frame = frames[index * (len(frames) - 1) // 15]
        # Use original pixels: downsampling can erase narrow but meaningful velocity arrows.
        atlas.paste(frame, ((index % 4) * width, (index // 4) * height))
    palette = atlas.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    encoded = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
    encoded[0].save(destination, save_all=True, append_images=encoded[1:], duration=50,
                    loop=0, optimize=True, disposal=1)


def package(raw_root: Path, output: Path, overwrite: bool) -> None:
    """Package only the named capture scenarios; require explicit permission to replace assets."""
    destinations = [output / filename for _, filename, _, _ in ASSETS]
    if not overwrite and any(path.exists() for path in destinations):
        raise FileExistsError("media already exists; use --overwrite to intentionally regenerate")
    output.mkdir(parents=True, exist_ok=True)
    for scene, filename, count, size in ASSETS:
        frames = read_frames(raw_root / scene, count, size)
        destination = output / filename
        if count == 1:
            frames[0].save(destination, optimize=True)
        else:
            encode_gif(frames, destination)
        print(f"{filename}: {destination.stat().st_size:,} bytes")


def verify(output: Path) -> None:
    """Decode every frame, enforce dimensions/playback/size, and reject empty or static GIFs."""
    for _, filename, count, size in ASSETS:
        path = output / filename
        if not 0 < path.stat().st_size <= 4 * 1024 * 1024:
            raise ValueError(f"{filename}: expected a nonempty asset no larger than 4 MiB")
        with Image.open(path) as image:
            expected_format = "PNG" if count == 1 else "GIF"
            if image.size != size or image.format != expected_format or image.n_frames != count:
                raise ValueError(f"{filename}: incorrect dimensions, format or frame count")
            if count > 1 and image.info.get("loop") != 0:
                raise ValueError(f"{filename}: animation must loop")
            first_frame = image.convert("RGB").tobytes()
            changed = False
            for index in range(count):
                image.seek(index)
                image.load()
                if count > 1 and image.info.get("duration") != 50:
                    raise ValueError(f"{filename}: expected 50 ms per frame")
                changed |= image.convert("RGB").tobytes() != first_frame
            if count > 1 and not changed:
                raise ValueError(f"{filename}: animation contains no movement")
        print(f"verified {filename}: {count} frame(s), {size[0]} x {size[1]}")


def main() -> int:
    """CLI entry point: package from raw frames, or verify already committed documentation media."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--frames", type=Path, help="root containing the five raw capture directories")
    parser.add_argument("--output", type=Path, default=Path("media/readme"))
    parser.add_argument("--overwrite", action="store_true")
    parser.add_argument("--verify", action="store_true", help="verify only; raw frames are not required")
    arguments = parser.parse_args()
    if arguments.verify == (arguments.frames is not None):
        parser.error("choose exactly one of --frames PATH or --verify")
    try:
        if arguments.frames is not None:
            package(arguments.frames, arguments.output, arguments.overwrite)
        verify(arguments.output)
        return 0
    except (OSError, ValueError) as error:
        print(f"media packaging failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
