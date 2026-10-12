#!/usr/bin/env python3
"""Encode supplied water screenshot pairs as looping GIFs; requires Pillow."""
from pathlib import Path
import json
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]

def main():
    data = json.loads((ROOT / 'docs/data/water-comparisons.json').read_text('utf-8'))
    for pair in data['pairs']:
        frames = [Image.open(ROOT / 'docs/media' / pair[key]).convert('RGB') for key in ('before', 'after')]
        if frames[0].size != frames[1].size:
            raise ValueError('Comparison screenshots must have identical dimensions')
        # One palette for both states avoids a palette shift when alternating.
        sample = Image.new('RGB', (frames[0].width, frames[0].height * 2))
        for index, frame in enumerate(frames):
            sample.paste(frame, (0, index * frame.height))
        palette = sample.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
        encoded = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
        dest = ROOT / 'docs/media' / pair['file']
        encoded[0].save(dest, save_all=True, append_images=encoded[1:], duration=data['frame_duration_ms'], loop=0, disposal=2, optimize=False)
        with Image.open(dest) as check:
            assert check.n_frames == 2 and check.info['loop'] == 0
            for index in range(2):
                check.seek(index)
                assert check.info['duration'] == data['frame_duration_ms']
        if dest.stat().st_size > 5 * 1024 * 1024:
            raise ValueError('Comparison exceeds documentation media size limit')
        print(f"{dest.relative_to(ROOT)}: 2 frames, {data['frame_duration_ms']} ms each, {dest.stat().st_size} bytes")

if __name__ == '__main__':
    main()
