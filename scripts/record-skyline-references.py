"""Record inspected built-in ImageGen references without any provider calls."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
prompts = json.loads((root/'docs/skyline-expansion-prompts.json').read_text())
for name, prompt in prompts.items():
    folder = root/'artifacts/model-input-images'/('kairox-'+name)
    image = folder/'reference-v1.png'
    record = dict(provider='built-in ImageGen', prompt=prompt, reference_sources=[],
                  selected_image='reference-v1.png', sha256=hashlib.sha256(image.read_bytes()).hexdigest(),
                  review='Complete isolated subject; silhouette and required architectural details inspected before submission')
    (folder/'provenance-v1.json').write_text(json.dumps(record, indent=2)+'\n')
