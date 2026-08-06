import argparse
import datetime as dt
import os
import re
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import numpy as np
from PIL import Image


DIRECTORY_PATTERN = re.compile(r"^(multiview_)\d{8}_\d{6}(_\d+)$")


def destination_directory(source: Path, now: dt.datetime) -> Path:
    match = DIRECTORY_PATTERN.fullmatch(source.name)
    if match is None:
        raise ValueError("source directory must be named multiview_YYYYMMDD_HHMMSS_NNN")
    return source.with_name(match.group(1) + now.strftime("%Y%m%d_%H%M%S") + match.group(2))


def transform_image(image: Image.Image, increment: int = 30, tolerance: int = 8) -> Image.Image:
    rgb = np.asarray(image.convert("RGB"), dtype=np.uint8)
    corners = np.array([rgb[0, 0], rgb[0, -1], rgb[-1, 0], rgb[-1, -1]], dtype=np.uint8)
    background = np.median(corners, axis=0).astype(np.int16)
    signed = rgb.astype(np.int16)
    non_background = np.max(np.abs(signed - background), axis=2) > tolerance
    output = signed.copy()
    output[non_background] = np.minimum(output[non_background] + increment, 255)
    return Image.fromarray(output.astype(np.uint8), mode="RGB")


def _process_image(
    source_file: Path,
    source_directory: Path,
    output_directory: Path,
    increment: int,
    tolerance: int,
) -> Path:
    output_file = output_directory / source_file.relative_to(source_directory)
    output_file.parent.mkdir(parents=True, exist_ok=True)
    with Image.open(source_file) as image:
        transform_image(image, increment, tolerance).save(
            output_file, "JPEG", quality=95, subsampling=0, optimize=False)
    return output_file


def brighten_directory(
    source: Path,
    increment: int = 30,
    tolerance: int = 8,
    now: dt.datetime | None = None,
    workers: int = 1,
) -> Path:
    source = source.resolve()
    if not source.is_dir():
        raise NotADirectoryError(f"source directory does not exist: {source}")
    if not 0 <= increment <= 255:
        raise ValueError("increment must be between 0 and 255")
    if not 0 <= tolerance <= 255:
        raise ValueError("tolerance must be between 0 and 255")
    if workers < 1:
        raise ValueError("workers must be at least 1")

    output = destination_directory(source, now or dt.datetime.now())
    if output.exists():
        raise FileExistsError(f"refusing to overwrite existing output directory: {output}")

    files = sorted(
        path for path in source.rglob("*")
        if path.is_file() and path.suffix.lower() in {".jpg", ".jpeg"})
    if not files:
        raise FileNotFoundError(f"no JPG files found in: {source}")

    output.mkdir(parents=True, exist_ok=False)
    completed = 0
    with ThreadPoolExecutor(max_workers=workers) as executor:
        futures = [
            executor.submit(_process_image, file, source, output, increment, tolerance)
            for file in files
        ]
        for future in as_completed(futures):
            future.result()
            completed += 1
            if completed % 500 == 0 or completed == len(files):
                print(f"Processed {completed}/{len(files)} JPG files", flush=True)
    return output


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Copy a multiview JPG directory and brighten non-background pixels.")
    parser.add_argument("source_directory", type=Path)
    parser.add_argument("--increment", type=int, default=30)
    parser.add_argument("--tolerance", type=int, default=8)
    parser.add_argument("--workers", type=int, default=max(1, os.cpu_count() or 1))
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        output = brighten_directory(
            args.source_directory, args.increment, args.tolerance, workers=args.workers)
    except (FileNotFoundError, FileExistsError, NotADirectoryError, ValueError, OSError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Output directory: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
