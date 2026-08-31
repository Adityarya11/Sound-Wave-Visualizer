"""Generate assets/icon.ico - a small aurora orb matching the visualiser.

Writes a multi-resolution 32-bit ICO by hand so the build needs no imaging
library. Run from the repository root:

    python tools/make_icon.py
"""

import math
import os
import struct

SIZES = (16, 24, 32, 48, 64, 128, 256)
SUPERSAMPLE = 4

# Same stops as assets/aurora.frag and src/core/aurora.hpp.
STOPS = (
    (0.106, 0.031, 0.259),
    (0.482, 0.184, 0.969),
    (0.780, 0.141, 0.694),
    (0.133, 0.827, 0.933),
    (0.486, 1.000, 0.796),
)
CORE_COLOUR = (0.86, 0.74, 1.0)

# A fixed "spectrum" so the icon reads as the same object at every size.
BANDS = (0.82, 0.44, 0.95, 0.30, 0.68, 0.52, 0.88, 0.36,
         0.74, 0.48, 0.92, 0.28, 0.60, 0.56, 0.80, 0.40)


def smoothstep(edge0, edge1, x):
    if edge0 == edge1:
        return 0.0 if x < edge0 else 1.0
    t = (x - edge0) / (edge1 - edge0)
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def palette(t):
    t = max(0.0, min(1.0, t)) * (len(STOPS) - 1)
    i = min(len(STOPS) - 2, int(t))
    f = t - i
    f = f * f * (3.0 - 2.0 * f)
    a, b = STOPS[i], STOPS[i + 1]
    return tuple(a[c] + (b[c] - a[c]) * f for c in range(3))


def band_at(angle):
    """Interpolate the fixed band table around the circle, mirrored."""
    m = 1.0 - abs(angle / math.pi)
    x = m * (len(BANDS) - 1)
    i = min(len(BANDS) - 2, int(x))
    f = x - i
    f = f * f * (3.0 - 2.0 * f)
    return BANDS[i] + (BANDS[i + 1] - BANDS[i]) * f


def sample(u, v):
    """Return straight (r, g, b, a) floats for a point in -0.5..0.5 space."""
    d = math.hypot(u, v)
    angle = math.atan2(u, v)

    spec = band_at(angle)
    core_r = 0.155
    edge = core_r + 0.175 * spec
    width = 0.020 + 0.045 * spec

    ring = math.exp(-((abs(d - edge) / width) ** 1.7))
    core = smoothstep(core_r, core_r - 0.05, d)
    halo = math.exp(-((max(d - core_r, 0.0) / 0.185) ** 1.5))

    glow = ring * 1.05 + core * 0.92 + halo * 0.30
    glow *= 1.0 - smoothstep(0.36, 0.495, d)

    hue = 0.30 + 0.95 * d + 0.10 * math.sin(angle * 3.0)
    colour = palette(abs((hue % 1.0) * 2.0 - 1.0))

    blend = smoothstep(core_r * 1.10, core_r * 0.20, d) * 0.80
    colour = tuple(colour[c] + (CORE_COLOUR[c] - colour[c]) * blend for c in range(3))

    return colour[0], colour[1], colour[2], max(0.0, min(1.0, glow))


def render(size):
    """Supersampled BGRA rows, bottom-up, straight alpha."""
    rows = []
    step = 1.0 / (size * SUPERSAMPLE)
    for y in range(size - 1, -1, -1):
        row = bytearray()
        for x in range(size):
            r = g = b = a = 0.0
            for sy in range(SUPERSAMPLE):
                for sx in range(SUPERSAMPLE):
                    u = (x * SUPERSAMPLE + sx + 0.5) * step - 0.5
                    v = (y * SUPERSAMPLE + sy + 0.5) * step - 0.5
                    sr, sg, sb, sa = sample(u, v)
                    # Weight colour by coverage so edges do not darken.
                    r += sr * sa
                    g += sg * sa
                    b += sb * sa
                    a += sa
            n = SUPERSAMPLE * SUPERSAMPLE
            if a > 1e-6:
                r, g, b = r / a, g / a, b / a
            a /= n
            q = lambda value: max(0, min(255, int(value * 255.0 + 0.5)))
            row += bytes((q(b), q(g), q(r), q(a)))
        rows.append(bytes(row))
    return b"".join(rows)


def image_blob(size):
    pixels = render(size)
    header = struct.pack(
        "<IiiHHIIiiII",
        40, size, size * 2, 1, 32, 0, len(pixels), 0, 0, 0, 0,
    )
    # AND mask: unused for 32-bit icons, but the format still requires it.
    mask_stride = ((size + 31) // 32) * 4
    mask = b"\x00" * (mask_stride * size)
    return header + pixels + mask


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target = os.path.join(root, "assets", "icon.ico")

    blobs = [image_blob(s) for s in SIZES]
    offset = 6 + 16 * len(SIZES)

    out = bytearray(struct.pack("<HHH", 0, 1, len(SIZES)))
    for size, blob in zip(SIZES, blobs):
        # 256 is stored as 0 in the directory entry.
        out += struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32,
                           len(blob), offset)
        offset += len(blob)
    for blob in blobs:
        out += blob

    with open(target, "wb") as handle:
        handle.write(bytes(out))
    print(f"wrote {target} ({len(out)} bytes, sizes {', '.join(map(str, SIZES))})")


if __name__ == "__main__":
    main()
