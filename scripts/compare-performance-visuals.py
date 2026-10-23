"""Compare matching before/after acceptance screenshots; no game mutation."""
from pathlib import Path
import json
from PIL import Image, ImageChops, ImageStat

root=Path(__file__).resolve().parents[1]
folder=root/'build/evidence/performance-fix'
report={}
for backend in ('webgpu','webgl2'):
    for pose in ('underpass','shops','bollards','plaza','paving-close'):
        before=Image.open(folder/f'{backend}-before-{pose}.png').convert('RGB')
        after=Image.open(folder/f'{backend}-after-{pose}.png').convert('RGB')
        assert before.size==after.size
        # Exclude the UI and FPS counter; inspect the same world rectangle.
        rectangle=(310,100,before.width,before.height-65)
        diff=ImageChops.difference(before.crop(rectangle),after.crop(rectangle))
        average=sum(ImageStat.Stat(diff).mean)/3
        pixels=getattr(diff,'get_flattened_data',diff.getdata)()
        changed=sum(max(pixel)>16 for pixel in pixels)/(diff.width*diff.height)
        assert average<1.5, f'{backend} {pose}: large mean image difference'
        assert changed<.01, f'{backend} {pose}: over 1% of world pixels changed substantially'
        report[f'{backend}-{pose}']={'meanAbsoluteChannelError255':average,'fractionPixelsChangingOver16':changed,'rectangle':rectangle}
        print(backend,pose,f'MAE {average:.4f}/255; substantial changes {changed:.4%}')
(folder/'visual-results.json').write_text(json.dumps(report,indent=2)+'\n')
