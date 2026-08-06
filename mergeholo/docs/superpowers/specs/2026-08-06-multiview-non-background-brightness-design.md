# Multiview Non-background Brightness Design

## Goal

Create a reusable PowerShell batch script that writes a brighter copy of one
multiview directory without altering the source images.

## Inputs and outputs

- Input: a directory named `multiview_YYYYMMDD_HHMMSS_NNN` containing JPGs.
- Output: a sibling directory with the same name pattern and a replacement
  current timestamp; the trailing sequence (`NNN`) is preserved.
- Each JPG retains its original filename.
- The source directory and files are never overwritten.

## Image rule

- Treat the corner colour as the uniform background reference.
- Pixels close to that background remain unchanged.
- Every other pixel has each RGB channel increased by 30, clamped to 255.
- This intentionally brightens the whole rendered subject, not only the face.

## Script behaviour

- Script accepts the source directory and an optional brightness increment.
- Default increment is 30.
- It fails before processing if the source naming pattern is invalid or the
  timestamp-replacement destination already exists.
- It reports the destination and image count after completion.
