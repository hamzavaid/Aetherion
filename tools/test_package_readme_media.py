"""Regression checks for optional media packaging; fixtures are never public demo assets."""

from pathlib import Path
import re
import tempfile
import unittest

from PIL import Image

from package_readme_media import encode_gif, package, read_frames


class MediaPackagingTests(unittest.TestCase):
    """Keep capture validation strict and meaningful narrow vector colors visible in GIFs."""

    def test_reads_ordered_lossless_frames(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            Image.new("RGB", (8, 6), (7, 11, 23)).save(root / "frame-000.ppm")
            frames = read_frames(root, 1, (8, 6))
            self.assertEqual(frames[0].getpixel((0, 0)), (7, 11, 23))

    def test_rejects_missing_extra_and_wrong_dimensions(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaises(ValueError):
                read_frames(root, 1, (8, 6))
            Image.new("RGB", (8, 6)).save(root / "frame-000.ppm")
            with self.assertRaises(ValueError):
                read_frames(root, 1, (9, 6))
            Image.new("RGB", (8, 6)).save(root / "frame-001.ppm")
            with self.assertRaises(ValueError):
                read_frames(root, 1, (8, 6))

    def test_requires_explicit_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = root / "workspace.png"
            target.write_bytes(b"existing asset")
            with self.assertRaises(FileExistsError):
                package(root / "missing", root, False)
            self.assertEqual(target.read_bytes(), b"existing asset")

    def test_animation_keeps_narrow_green_arrow_color(self):
        frames = []
        for index in range(16):
            frame = Image.new("RGB", (32, 24), (12, 12, 20))
            frame.putpixel((index, 12), (0, 255, 0))
            frame.putpixel((index, 13), (200, 80, 255))
            frames.append(frame)
        with tempfile.TemporaryDirectory() as directory:
            destination = Path(directory) / "test.gif"
            encode_gif(frames, destination)
            with Image.open(destination) as animation:
                self.assertEqual(animation.n_frames, 16)
                self.assertEqual(animation.info["loop"], 0)
                for index in range(16):
                    animation.seek(index)
                    self.assertEqual(animation.info["duration"], 50)
                    self.assertEqual(animation.convert("RGB").getpixel((index, 12)),
                                     (0, 255, 0))

    def test_public_documentation_links_and_anchors_resolve(self):
        root = Path(__file__).resolve().parents[1]
        for name in ("README.md", "GUIDE.md", "TECHNICAL_GUIDE.md", "media/readme/README.md"):
            source = root / name
            text = source.read_text(encoding="utf-8")
            for target in re.findall(r"\]\(([^)]+)\)", text):
                if "://" in target:
                    continue
                filename, _, anchor = target.partition("#")
                path = (source.parent / filename).resolve() if filename else source
                with self.subTest(source=name, target=target):
                    self.assertTrue(path.exists())
                    self.assertNotIn("docs", path.relative_to(root).parts)
                    if anchor:
                        headings = re.findall(r"^#+ (.+)$", path.read_text(encoding="utf-8"), re.M)
                        slugs = [re.sub(r"[^\w -]", "", heading.lower()).replace(" ", "-")
                                 for heading in headings]
                        self.assertIn(anchor, slugs)


if __name__ == "__main__":
    unittest.main()
