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
        print("usage: python run_generateimages_equiv.py <input_dir> <output_dir>")
        return 2

    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    dst.mkdir(parents=True, exist_ok=True)

    progress_path = dst / "generateimages_progress.json"
    temp_path = dst / "_views_270x270_res150.uint8.memmap"

    grid_h = 270
    grid_w = 270
    image_h = 150
    image_w = 150
    expected_inputs = grid_h * grid_w
    expected_outputs = image_h * image_w

    input_files = [src / f"{r:03d}{c:03d}.jpg" for r in range(1, grid_h + 1) for c in range(1, grid_w + 1)]
    missing = [p.name for p in input_files if not p.exists()]
    if missing:
        write_progress(progress_path, stage="failed", error="missing input files", missing_count=len(missing), first_missing=missing[:20])
        raise RuntimeError(f"missing {len(missing)} input files, first: {missing[:5]}")

    for old in dst.glob("*.jpg"):
        old.unlink()
    if temp_path.exists():
        temp_path.unlink()

    write_progress(
        progress_path,
        stage="loading_inputs",
        input_dir=str(src),
        output_dir=str(dst),
        expected_inputs=expected_inputs,
        expected_outputs=expected_outputs,
        loaded=0,
        written=0,
    )

    views = np.memmap(temp_path, dtype=np.uint8, mode="w+", shape=(grid_h, grid_w, image_h, image_w, 3))

    loaded = 0
    start = time.time()
    for r in range(grid_h):
        for c in range(grid_w):
            file_path = src / f"{r + 1:03d}{c + 1:03d}.jpg"
            img = cv2.imread(str(file_path), cv2.IMREAD_COLOR)
            if img is None:
                raise RuntimeError(f"failed to read {file_path}")
            if img.shape[:2] != (image_h, image_w):
                raise RuntimeError(f"unexpected size for {file_path}: {img.shape[:2]}")
            views[r, c] = img
            loaded += 1
        views.flush()
        write_progress(
            progress_path,
            stage="loading_inputs",
            loaded=loaded,
            loaded_rows=r + 1,
            expected_inputs=expected_inputs,
            written=0,
            elapsed_sec=round(time.time() - start, 3),
        )

    write_progress(
        progress_path,
        stage="writing_outputs",
        loaded=loaded,
        expected_inputs=expected_inputs,
        written=0,
        expected_outputs=expected_outputs,
        elapsed_sec=round(time.time() - start, 3),
    )

    params = [int(cv2.IMWRITE_JPEG_QUALITY), 100]
    written = 0
    for r in range(image_h):
        for c in range(image_w):
            out_img = np.ascontiguousarray(views[:, :, r, c, :])
            out_path = dst / f"{r + 1:03d}{c + 1:03d}.jpg"
            if not cv2.imwrite(str(out_path), out_img, params):
                raise RuntimeError(f"failed to write {out_path}")
            written += 1
        write_progress(
            progress_path,
            stage="writing_outputs",
            loaded=loaded,
            expected_inputs=expected_inputs,
            written=written,
            written_rows=r + 1,
            expected_outputs=expected_outputs,
            elapsed_sec=round(time.time() - start, 3),
        )

    views.flush()
    write_progress(
        progress_path,
        stage="complete",
        loaded=loaded,
        expected_inputs=expected_inputs,
        written=written,
        expected_outputs=expected_outputs,
        temp_path=str(temp_path),
        elapsed_sec=round(time.time() - start, 3),
    )
    print(f"complete: loaded={loaded}, written={written}, output={dst}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
