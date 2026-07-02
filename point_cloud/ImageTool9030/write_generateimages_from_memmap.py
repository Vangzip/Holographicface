import json
import sys
import time
from pathlib import Path

import cv2
import numpy as np


def write_progress(path, **data):
    data["updated_at"] = time.strftime("%Y-%m-%d %H:%M:%S")
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


def main():
    if len(sys.argv) != 3:
        print("usage: python write_generateimages_from_memmap.py <memmap_path> <output_dir>")
        return 2

    memmap_path = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    progress_path = dst / "generateimages_progress.json"

    grid_h = 270
    grid_w = 270
    image_h = 150
    image_w = 150
    expected_outputs = image_h * image_w
    chunk_rows = 9

    if not memmap_path.exists():
        raise RuntimeError(f"missing memmap: {memmap_path}")

    for old in dst.glob("*.jpg"):
        old.unlink()

    views = np.memmap(memmap_path, dtype=np.uint8, mode="r", shape=(grid_h, grid_w, image_h, image_w, 3))
    params = [int(cv2.IMWRITE_JPEG_QUALITY), 100]

    start = time.time()
    written = 0
    write_progress(
        progress_path,
        stage="writing_outputs_chunked",
        written=written,
        expected_outputs=expected_outputs,
        chunk_rows=chunk_rows,
        elapsed_sec=0,
    )

    for r0 in range(0, image_h, chunk_rows):
        r1 = min(image_h, r0 + chunk_rows)
        # Source layout is view-row, view-col, image-row, image-col, channel.
        # This transpose makes a small batch of output images contiguous:
        # image-row, image-col, view-row, view-col, channel.
        chunk = np.ascontiguousarray(views[:, :, r0:r1, :, :].transpose(2, 3, 0, 1, 4))
        for dr in range(r1 - r0):
            out_r = r0 + dr + 1
            for c in range(image_w):
                out_path = dst / f"{out_r:03d}{c + 1:03d}.jpg"
                if not cv2.imwrite(str(out_path), chunk[dr, c], params):
                    raise RuntimeError(f"failed to write {out_path}")
                written += 1
        write_progress(
            progress_path,
            stage="writing_outputs_chunked",
            written=written,
            written_rows=r1,
            expected_outputs=expected_outputs,
            chunk_rows=chunk_rows,
            elapsed_sec=round(time.time() - start, 3),
        )

    write_progress(
        progress_path,
        stage="complete",
        written=written,
        expected_outputs=expected_outputs,
        temp_path=str(memmap_path),
        elapsed_sec=round(time.time() - start, 3),
    )
    print(f"complete: written={written}, output={dst}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
