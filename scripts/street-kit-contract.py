"""Write C++ hash/bounds contract from prepared street assets; no provider calls."""
import hashlib
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
NAMES=['transit-pillar','transit-beam','transit-deck','station','shop-shutter','shop-doorway','shop-awning','shop-sign','safety-fence','spectator-railing','charging-pedestal','utility-cabinet']
rows=[]
for short in NAMES:
    name='kairox-'+short
    folder=ROOT/'assets/runtime/models/street-kit'/name
    data=json.loads((folder/(name+'.asset.json')).read_text())
    assert hashlib.sha256((folder/(name+'.glb')).read_bytes()).hexdigest()==data['sha256']
    dimensions=','.join(f'{value:.7f}f' for value in data['dimensions'])
    rows.append('{"'+name+'","'+data['sha256']+'",'+str(data['triangles'])+',{'+dimensions+'}},')
header='#pragma once\n#include "engine/math.hpp"\n#include <array>\nnamespace racing::streetkit {\nstruct Model {const char* name;const char* hash;unsigned triangles;engine::Vec3 dimensions;};\ninline const std::array<Model,12> models{{\n'+'\n'.join(rows)+'\n}};\n}\n'
(ROOT/'src/street_art_contract.hpp').write_text(header)
print('Wrote 12 verified street asset contracts')
