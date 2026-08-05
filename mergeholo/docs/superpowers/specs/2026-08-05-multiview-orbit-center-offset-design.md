# Multiview orbit-center forward offset

## Goal

Make all generated multiview cameras orbit an explicit, configurable target
instead of the mesh's raw bounding-sphere center.  The initial trial moves the
target 0.10 model units in positive Z, toward the reconstructed head and away
from posterior fragments.

## Scope

- Preserve the existing mesh-centering transform.
- Keep all point-cloud filtering and mesh-reconstruction behavior unchanged.
- Set the default pipeline trial value to
  `multiview_camera_center_offset_z=0.10`.
- Apply the configured center consistently to initial camera setup, batch
  rendering, and atlas rendering.

## Design

`modelMoveHandler` already establishes the initial camera target as the
centered model center plus `ModelMoveCameraConfig::centerOffset`.  The batch
and atlas renderers must retain that target rather than replacing it with
`modelTransform_->getBound().center()`.

For each renderer, read the current camera look-at tuple once before building
the orbit basis.  Use `viewCenter` as `orbitCenter`, calculate the eye
direction and distance from `eye - orbitCenter`, and pass that same center to
every `orbitViewMatrix` call.  Consequently every generated image uses a
camera position and look-at target defined around the offset center.

## Verification

Extend the multiview orbit tests with a nonzero target.  They must establish
that every generated view matrix's look-at target equals the configured
offset target, while the old raw mesh-bound center differs.  Run the focused
test target and its existing multiview/pipeline test suite.

## Expected trial setting

The latest captured mesh's dominant head component is centered around
`z=-0.575`, whereas fragments extend the full mesh bounding-box center to
about `z=-0.675`.  Therefore, after centering the mesh, a `+0.10` Z target
offset is the initial reversible trial.  It is a rendering pivot adjustment,
not a claim that the result is an anatomical ear-plane estimate.

## Non-goals

- Removing point-cloud or mesh fragments.
- Automatically detecting ears, landmarks, or a head center.
- Changing capture orientation, camera FOV, or reconstruction parameters.
