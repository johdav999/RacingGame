# Textured cybercar parts

`cybercar1_parts_textured.glb` retains all 16 segmented meshes (9,162 triangles).
The original `cybercar1.glb` supplies the baked appearance; the original
`cybercar1_parts.glb` supplies the geometry. Both inputs remain unchanged.
One shared PBR material uses embedded 4096 × 4096 PNG maps for base color,
metallic/roughness (G = roughness, B = metallic), and tangent-space normals.

## Wheel animation

The four independently rotating mesh nodes are:

- `wheel_front_left`
- `wheel_front_right`
- `wheel_rear_left`
- `wheel_rear_right`

Their origins are centered on their tire silhouettes and midway across their
width. Rotate each node about its **local Z axis in glTF coordinates**.
The GLB uses **+Y up, -X forward**. Blender converts this spin axis to local -Y
when importing. Left/right are defined from the driver's forward direction.
Keep each wheel's initial node translation when animating.

For steering, add a parent pivot at the front wheel's center and rotate that
parent around the vehicle up axis; apply wheel spin to the wheel child.
The delivered file contains no animation clips or steering parents.

The source's normalized dimensions are preserved (approximately 0.98 units
long). Apply any game-scale conversion uniformly to the `cybercar1` root.
Exact pivot positions and estimated tire radii are recorded in
`cybercar1_parts_textured.asset.json`.

## Production and verification

Textures were transferred with Blender 3.5.1 Cycles selected-to-active baking,
using a new shared UV atlas. The two input geometries differ, so local projected
texture distortion remains; this is not an exact topology-preserving transfer.

Fresh GLB reimport confirmed all 16 meshes, UVs, one shared material, and three
embedded texture files matching the bake PNGs byte for byte. Tests at
45, 90, 180, 270 and 360 degrees confirmed fixed wheel centers and unchanged
body geometry. Front, rear and rotated-wheel renders were inspected.
The RacingGame player integration is now verified in native simulation and both
browser renderers; see [game binding and validation](../../../../docs/player-car.md).

Editable packed Blender source, bake scripts, texture PNGs, renders and the full
verification report are under
`artifacts/model-jobs/cybercar1-texture-transfer/` (ignored working artifacts).
The main editable file is `cybercar1_parts_textured.blend`.
The runtime provenance manifest is `cybercar1_parts_textured.asset.json`.
