# Garage body paint

Choose a chassis, then use BODY PAINT to open the browser colour picker. Changes preview on the player car in the garage; ORIGINAL restores its authored finish. Use VIEW CAR to collapse the garage to the paint controls, including on a narrow screen; BACK TO GARAGE restores it. Each of Vektor, Skimmer, Bastion and Vector retains its own colour. Paint is free and available in both race and time-trial garages. Customization locks when an event starts.

`src/vehicle_paint.hpp` builds a soft grayscale paint mask once per chassis from its neutral body panels: minimum RGB brightness ramps from 0.28 to 0.58 with smoothstep, while RGB spread fades mask coverage from 0.035 to 0.155. Dark glass/trim and saturated accents therefore receive zero coverage; intermediate edges blend. These are generated neutral-panel masks, not hand-segmented material IDs; changing source art requires reviewing the mask and preview again.

Masked texels receive the selected colour multiplied by source brightness, preserving panel shading. Unmasked texels retain the prepared albedo. Wheels, wheel wells and mounted equipment retain their material bindings. A player-only material instance overrides body albedo; shared mesh/material assets and rival appearances remain immutable. Customized albedo is capped at 1024 pixels per dimension to bound interactive recomposition and texture uploads. ORIGINAL restores the original texture, including Vektor's 4K source. Normal and metallic/roughness maps remain authored.

The closer garage camera shows the selected car; entering a normal or league race restores the chase camera. Picker input coalesces rapid changes over 60 ms and commits the final selection on change.

The bounded NA5 career format (reading NA1–NA4) preserves league records and appends four RGB integers (0..16777215); 16777216 means original. NA1, NA2 and NA3 saves migrate with original finishes. Decode stages the entire career before replacement. Invalid colours, extra fields and oversized saves reject without replacing current progress. Existing storage-conflict and unavailable-storage behavior applies to paint as well as upgrades.

Validation: `racing_paint` exercises all four imported chassis, protected texels, immutable source textures, four-colour round trips, NA3 migration, corrupt-load preservation, reset and event lock. `scripts/paint-smoke.cjs` runs the generated site with its own temporary local server on WebGPU and WebGL2, checks visible preview changes, all four saved colours after reload, reset, event lock and narrow layout. Evidence is written under `build/evidence/paint`.

## Verified 2026-10-23

Native `racing_paint`, `racing_championship`, and the full `racing_smoke` passed. The existing native regression run also passed the remaining 19 selected checks; its two save-version assertion failures were corrected and rerun successfully. The paint browser report records both actual backends passing against Wasm SHA-256 `89f83aa0f8afbcfc627ea33c1a6d8afc827ea240e1d5cfcd34b9131572b4ba69`, pinned engine `9b597da84d4582973d010fa9ba3e328c20181892`, with no page errors. Screenshots include four desktop chassis previews and the compact 390 x 844 preview for each backend. Browser viewport automation does not establish physical phone colour-picker acceptance.
