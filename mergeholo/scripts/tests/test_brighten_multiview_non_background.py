import datetime as dt
import tempfile
import unittest
from pathlib import Path

import numpy as np
from PIL import Image

from scripts.brighten_multiview_non_background import (
    brighten_directory,
    destination_directory,
    transform_image,
)


class BrightenMultiviewTests(unittest.TestCase):
    def test_transform_preserves_background_and_brightens_subject(self):
        pixels = np.full((3, 3, 3), 77, dtype=np.uint8)
        pixels[1, 1] = (80, 100, 220)
        image = Image.fromarray(pixels, mode="RGB")

        actual = np.asarray(transform_image(image, increment=30, tolerance=8))

        self.assertTrue(np.array_equal(actual[0, 0], (77, 77, 77)))
        self.assertTrue(np.array_equal(actual[1, 1], (110, 130, 250)))

    def test_transform_clamps_to_white(self):
        pixels = np.full((3, 3, 3), 77, dtype=np.uint8)
        pixels[1, 1] = (250, 240, 230)

        actual = np.asarray(transform_image(Image.fromarray(pixels, mode="RGB"), 30, 8))

        self.assertTrue(np.array_equal(actual[1, 1], (255, 255, 255)))

    def test_destination_replaces_only_timestamp(self):
        source = Path("C:/work/multiview_20260806_104704_011")

        actual = destination_directory(source, dt.datetime(2026, 8, 6, 12, 34, 56))

        self.assertEqual(actual.name, "multiview_20260806_123456_011")

    def test_brighten_directory_preserves_filename_without_touching_source(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "multiview_20260806_104704_011"
            source.mkdir()
            original = Image.new("RGB", (4, 4), (77, 77, 77))
            original.putpixel((2, 2), (100, 100, 100))
            source_file = source / "097142.jpg"
            original.save(source_file, quality=100)
            source_bytes = source_file.read_bytes()

            output = brighten_directory(source, increment=30, tolerance=8,
                now=dt.datetime(2026, 8, 6, 12, 34, 56), workers=1)

            self.assertEqual(output.name, "multiview_20260806_123456_011")
            self.assertTrue((output / "097142.jpg").is_file())
            self.assertEqual(source_file.read_bytes(), source_bytes)


if __name__ == "__main__":
    unittest.main()
