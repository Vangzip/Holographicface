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
        print("usage: python run_generateimages_stream_chunks.py <input_dir> <output_dir>")
        return 2

    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    dst.mkdir(parents=True, exist_ok=True)

    grid_h = 270
    grid_w = 270
    image_h = 150
    image_w = 150
    chunk_rows = 20
    expected_inputs = grid_h * grid_w
    expected_outputs = image_h * image_w
    progress_path = dst / "generateimages_progress.json"

    input_files = [src / f"{r:03d}{c:03d}.jpg" for r in range(1, grid_h + 1) for c in range(1, grid_w + 1)]
    missing = [p.name for p in input_files if not p.exists()]
    if missing:
        write_progress(progress_path, stage="failed", error="missing input files", missing_count=len(missing), first_missing=missing[:20])
        raise RuntimeError(f"missing {len(missing)} input files, first: {missing[:5]}")

    for old in dst.glob("*.jpg"):
        old.unlink()

    params = [int(cv2.IMWRITE_JPEG_QUALITY), 100]
    start = time.time()
    written = 0
    write_progress(
        progress_path,
        stage="stream_chunk_outputs",
        chunk_rows=chunk_rows,
        written=0,
        expected_inputs=expected_inputs,
        expected_outputs=expected_outputs,
        elapsed_sec=0,
    )

    for r0 in range(0, image_h, chunk_rows):
        r1 = min(image_h, r0 + chunk_rows)
        rows = r1 - r0
        # Layout: image-row-in-chunk, image-col, view-row, view-col, channel.
        outputs = np.empty((rows, image_w, grid_h, grid_w, 3), dtype=np.uint8)
        loaded = 0
        for vr in range(grid_h):
            for vc in range(grid_w):
                file_path = src / f"{vr + 1:03d}{vc + 1:03d}.jpg"
                img = cv2.imread(str(file_path), cv2.IMREAD_COLOR)
                if img is None:
                    raise RuntimeError(f"failed to read {file_path}")
                if img.shape[:2] != (image_h, image_w):
                    raise RuntimeError(f"unexpected size for {file_path}: {img.shape[:2]}")
                outputs[:, :, vr, vc, :] = img[r0:r1, :, :]
                loaded += 1
        for dr in range(rows):
            out_r = r0 + dr + 1
            for c in range(image_w):
                out_path = dst / f"{out_r:03d}{c + 1:03d}.jpg"
                if not cv2.imwrite(str(out_path), outputs[dr, c], params):
                    raise RuntimeError(f"failed to write {out_path}")
                written += 1
        write_progress(
            progress_path,
            stage="stream_chunk_outputs",
            chunk_rows=chunk_rows,
            loaded_for_current_chunk=loaded,
            written=written,
            written_rows=r1,
            expected_inputs=expected_inputs,
            expected_outputs=expected_outputs,
            elapsed_sec=round(time.time() - start, 3),
        )

    write_progress(
        progress_path,
        stage="complete",
        chunk_rows=chunk_rows,
        written=written,
        expected_outputs=expected_outputs,
        elapsed_sec=round(time.time() - start, 3),
    )
    print(f"complete: written={written}, output={dst}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
