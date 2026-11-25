"""Compare native Photoshop TIFF exports from the disposable texture-wrap cases."""
from pathlib import Path
import argparse
import struct

from tiff_pixels import tiff


def verify(folder, check_reopen, legacy_root, previous_root, baseline_root, self_cylinder=False, prior_cylinder_root=None):
    for bits, sample in ((8, 'B'), (16, 'H'), (32, 'f')):
        def load(name):
            e, w, h, planes, depth, data = tiff(folder / f'{name}-{bits}.tif')
            assert (w, h, planes, depth) == (193, 161, 4, bits), (name, bits, w, h, planes, depth)
            return data, struct.Struct(e + sample * 4)

        source, _ = load('source')
        ordinary, pixel = load('ordinary')
        undo, _ = load('undo')
        selected, _ = load('selection')
        assert undo == source, (folder, bits, 'undo')
        assert ordinary != source, (folder, bits, 'effect is active')

        maximum = 1 if bits == 32 else (1 << bits) - 1
        r, g, b, a = pixel.unpack_from(ordinary, (80 * 193 + 96) * pixel.size)
        assert a == maximum and g > max(r, b), (folder, bits, 'center samples the green texture', r, g, b, a)
        assert not any(pixel.unpack_from(ordinary, 0)), (folder, bits, 'transparent exterior')
        assert any(0 < p[3] < maximum for p in pixel.iter_unpack(ordinary)), (folder, bits, 'coverage antialiasing')

        def at(data, x, y):
            return pixel.unpack_from(data, (y * 193 + x) * pixel.size)

        local, _ = load('local')
        assert at(local, 96, 80)[3] == maximum, (folder, bits, 'local texture is visible')
        external, _ = load('external')
        r, g, b, a = at(external, 96, 80)
        assert a == maximum and b > max(r, g), (folder, bits, 'external texture sampled from off-canvas pixels', r, g, b, a)
        overlay, _ = load('overlay')
        assert at(overlay, 96, 80) == at(external, 96, 80), (folder, bits, 'opaque texture overlays the center')
        outside = (32, 80) if folder.name == 'cylinder' else (96, 110)
        assert at(external, *outside)[3] == 0 and at(overlay, *outside) == at(ordinary, *outside), (folder, bits, 'base remains outside the sticker')
        assert at(overlay, 0, 0)[3] == 0, (folder, bits, 'transparent exterior with both textures')
        assert load('reference-before')[0] == load('reference-after')[0], (folder, bits, 'external layer pixels are unchanged')
        assert load('current-again')[0] == local, (folder, bits, 'current layer mode matches the default')
        if folder.name == 'cylinder':
            assert at(ordinary, 32, 80)[3] == maximum and at(local, 32, 80)[3] == 0, (bits, '120° exterior is transparent')
            decal, _ = load('positioned-decal')
            assert at(decal, 140, 56)[3] == maximum, (bits, 'decal follows its azimuth and height position')
            assert at(decal, 96, 80)[3] == 0 and at(decal, 140, 80)[3] == 0, (bits, 'decal clears the old center and lower wall')
            if self_cylinder:
                self_overlay, _ = load('self-overlay')
                assert at(self_overlay, 96, 80) == at(ordinary, 96, 80), (bits, 'same-layer base behind the decal')
                assert at(self_overlay, 140, 56) == at(decal, 140, 56), (bits, 'positioned same-layer decal')
                assert at(self_overlay, 0, 0)[3] == 0, (bits, 'cylinder exterior stays transparent')
                assert self_overlay != ordinary, (bits, 'same-layer decal changes the full projection')
            front, _ = load('tilted-front')
            back, _ = load('tilted-back')
            both, _ = load('tilted-both')
            assert at(front, 96, 30)[3] == 0 and at(back, 96, 30)[3] == maximum, (bits, 'back wall through opening')
            assert at(both, 96, 30) == at(back, 96, 30), (bits, 'both shows back at opening')
            assert at(front, 96, 80)[3] == maximum and at(front, 96, 80) != at(back, 96, 80), (bits, 'opposite texture points')
            assert at(both, 96, 80) == at(front, 96, 80), (bits, 'front hides the back where opaque')
        else:
            assert at(ordinary, 96, 110)[3] == maximum and at(local, 96, 110)[3] == 0, (bits, 'outside the decal is transparent')
            self_overlay, _ = load('self-overlay')
            assert at(self_overlay, 96, 110) == at(ordinary, 96, 110), (bits, 'current texture remains under the decal')
            assert at(self_overlay, 96, 80) == at(local, 96, 80), (bits, 'current texture overlays the sphere as a decal')
            assert at(self_overlay, 0, 0)[3] == 0, (bits, 'self overlay leaves the sphere exterior transparent')
            moved, _ = load('moved-decal')
            assert at(moved, 139, 59)[3] == maximum and at(moved, 96, 80)[3] == 0, (bits, 'decal follows its surface position')

        for y in range(161):
            start, split, end = [(y * 193 + x) * pixel.size for x in (0, 103, 193)]
            actual, expected = selected[start:split], ordinary[start:split]
            if bits == 32 and actual != expected:
                # Photoshop composites selected floating-point pixels through its auto-mask path.
                assert all(abs(a - b) <= 1e-7 for left, right in zip(pixel.iter_unpack(actual), pixel.iter_unpack(expected))
                           for a, b in zip(left, right)), (folder, bits, 'selection keeps the full texture', y)
            else:
                assert actual == expected, (folder, bits, 'selection keeps the full texture', y)
            assert selected[split:end] == source[split:end], (folder, bits, 'selection preserves outside pixels', y)

        if check_reopen:
            smart, _ = load('smart')
            reopened, _ = load('reopened')
            forced, _ = load('forced')
            assert smart != source, (folder, bits, 'smart filter is active')
            assert smart == reopened == forced, (folder, bits, 'PSD reopen and explicit recalculation')
            if legacy_root:
                prior = tiff(legacy_root / folder.name / f'forced-{bits}.tif')
                assert load('legacy')[0] == prior[-1], (folder, bits, '0.3 PSD replay')
            if previous_root:
                prior = tiff(previous_root / folder.name / f'forced-{bits}.tif')
                assert load('previous')[0] == prior[-1], (folder, bits, '0.4 external-source PSD replay')
            if baseline_root:
                prior = tiff(baseline_root / folder.name / f'forced-{bits}.tif')
                assert load('baseline')[0] == prior[-1], (folder, bits, '0.4.1 overlay PSD replay')
            if prior_cylinder_root and folder.name == 'cylinder':
                prior = tiff(prior_cylinder_root / folder.name / f'forced-{bits}.tif')
                assert load('prior-cylinder')[0] == prior[-1], (folder, bits, '0.5 positioned cylinder PSD replay')

    _, w, h, planes, depth, raw = tiff(folder / 'offcanvas-8.tif')
    assert (w, h, planes, depth) == (193, 161, 4, 8)
    start = (80 * w + 96) * 4
    r, g, b, a = raw[start:start + 4]
    assert a == 255 and r > 150 and g < 40 and b < 40, (folder, 'center samples texture beyond canvas', r, g, b, a)
    print(f'PASS {folder.name}: RGB8/16/32, alpha, undo, selection, hidden off-canvas texture, overlay, source pixels' +
          (', PSD reopen and recalculate' if check_reopen else ''))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('folders', type=Path, nargs='+')
    parser.add_argument('--create-only', action='store_true')
    parser.add_argument('--legacy-root', type=Path)
    parser.add_argument('--previous-root', type=Path)
    parser.add_argument('--baseline-root', type=Path)
    parser.add_argument('--cylinder-self-overlay', action='store_true')
    parser.add_argument('--prior-cylinder-root', type=Path)
    args = parser.parse_args()
    for case in args.folders:
        verify(case, not args.create_only, args.legacy_root, args.previous_root, args.baseline_root,
               args.cylinder_self_overlay, args.prior_cylinder_root)
