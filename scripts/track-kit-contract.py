"""Pin the thirteen prepared runtime GLBs and expose their verified metrics."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
specs=json.loads((root/'docs/track-kit-spec.json').read_text())
rows=[]
for spec in specs:
    name=spec['name'];record=json.loads((root/'assets/runtime/models/track-kit'/name/(name+'.asset.json')).read_text())
    dimensions=','.join(f'{d:.6f}f' for d in record['dimensions'])
    rows.append('{"%s","%s",%s,{%s}},'%(name,record['sha256'],record['triangles'],dimensions))
(root/'src/track_art_contract.hpp').write_text('''#pragma once
#include "engine/math.hpp"
#include <array>
namespace racing::trackkit {
struct Model { const char* name; const char* hash; unsigned triangles; engine::Vec3 dimensions; };
inline const std::array<Model,13> models{{
'''+ '\n'.join(rows)+'\n}};\n}\n')
