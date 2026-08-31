// Aurora orb - the circular visualiser is drawn entirely here, as one
// full-window pass. Doing it in a shader (rather than N rotated rectangles)
// is what gives the soft bloom; geometry alone can only produce hard edges.
//
// Edit this file and restart the app to retune the look - a copy sits next to
// visualizer.exe and overrides the version compiled into the binary.

uniform vec2      u_resolution;  // window size in pixels
uniform float     u_time;        // seconds since launch
uniform float     u_beat;        // 0..1 impulse, spikes on bass transients
uniform float     u_level;       // 0..1 overall loudness
uniform float     u_opacity;     // 0..1 master fade (drops to 0 when silent)
uniform float     u_hue;         // palette rotation, drifts slowly
uniform sampler2D u_spec;        // 1-row texture, red channel = band magnitude

const float PI = 3.14159265;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; ++i)
    {
        v += a * noise(p);
        p *= 2.02;
        a *= 0.5;
    }
    return v;
}

// Deep indigo -> electric violet -> magenta -> cyan -> mint.
// Violet occupies the middle two stops so it dominates the mix.
vec3 palette(float t)
{
    t = clamp(t, 0.0, 1.0) * 4.0;
    vec3 c0 = vec3(0.106, 0.031, 0.259);
    vec3 c1 = vec3(0.482, 0.184, 0.969);
    vec3 c2 = vec3(0.780, 0.141, 0.694);
    vec3 c3 = vec3(0.133, 0.827, 0.933);
    vec3 c4 = vec3(0.486, 1.000, 0.796);

    vec3 a = c0;
    vec3 b = c1;
    if (t >= 3.0)      { a = c3; b = c4; t -= 3.0; }
    else if (t >= 2.0) { a = c2; b = c3; t -= 2.0; }
    else if (t >= 1.0) { a = c1; b = c2; t -= 1.0; }

    t = t * t * (3.0 - 2.0 * t);
    return mix(a, b, t);
}

void main()
{
    vec2  centre = u_resolution * 0.5;
    vec2  p      = gl_FragCoord.xy - centre;
    float base   = min(u_resolution.x, u_resolution.y);

    // Normalised radius: 0.5 is the nearest window edge, so every constant
    // below is a fraction of the window and the orb scales with it for free.
    float len = length(p);
    float d   = len / base;

    // Unit direction doubles as a seamless angular coordinate. Anything driven
    // by a 0..1 angle instead would tear along the wrap, because noise(0) and
    // noise(1) are unrelated values.
    vec2 dir = p / max(len, 1e-4);

    // atan(x, y) measures from straight up, and taking its magnitude mirrors
    // the left half onto the right. Bass lands at the bottom, treble at the
    // top, and both poles are continuous so the ring has no seam.
    float phi  = atan(p.x, p.y);
    float m    = 1.0 - abs(phi) / PI;
    float spec = texture2D(u_spec, vec2(m, 0.5)).r;

    // Break up the perfect symmetry a little, or it reads as mechanical.
    spec *= 0.85 + 0.30 * fbm(dir * 4.5 + vec2(u_time * 0.45));

    float R    = 0.150 + 0.030 * u_beat + 0.014 * u_level;  // breathing core
    float edge = R + 0.155 * spec;                          // spectrum rim
    float w    = 0.006 + 0.030 * spec;                      // rim thickness

    float ring  = exp(-pow(abs(d - edge) / w, 1.7));
    float core  = smoothstep(R, R - 0.055, d);
    float inner = exp(-pow(max(d - R, 0.0) / 0.075, 2.0));
    float halo  = exp(-pow(max(d - R, 0.0) / 0.200, 1.5));

    float energy = 0.35 + 0.65 * u_level;
    float glow   = ring * (0.85 + 0.60 * spec)
                 + inner * 0.42
                 + halo * 0.28 * energy
                 + core * 0.38;

    // The window is a rectangle but the glow is radial. Without this taper the
    // halo gets sliced off at the window edges and the whole overlay reads as
    // a faint grey box sitting on the desktop.
    glow *= 1.0 - smoothstep(0.36, 0.50, d);

    // Drifting curtains. Ping-ponging the palette index (rather than wrapping)
    // keeps the gradient seamless as u_hue rolls past 1.0.
    float t = u_time * 0.06;

    // Every angular term has to fade out near the middle. dir is meaningless
    // as d approaches zero, so leaving it live paints a dark pinwheel of
    // streaks converging on the centre pixel.
    float swirl = smoothstep(0.0, 0.10, d);

    float n    = fbm(dir * 2.2 + vec2(d * 3.4) + vec2(t * 1.4, -t * 1.1));
    float hueT = u_hue + 0.30 * n * swirl + 0.55 * d + 0.05 * u_beat
               + 0.16 * (0.5 + 0.5 * sin(phi + u_time * 0.15)) * swirl;
    vec3  col  = palette(abs(fract(hueT) * 2.0 - 1.0));

    // The indigo end of the ramp is right for the rim and wrong for the core:
    // an opaque dark centre reads as a hole punched in the screen rather than
    // a light source. Fade the middle to a pale violet instead.
    col = mix(col, vec3(0.86, 0.74, 1.0), smoothstep(R * 1.10, R * 0.20, d) * 0.78);

    // Transients bleach the rim towards white so beats land as flashes. Kept
    // small: a larger factor washes the whole palette out to pink.
    col = mix(col, vec3(1.0), 0.22 * u_beat * ring);

    float alpha = clamp(glow, 0.0, 1.0) * u_opacity;

    // Only a gentle lift with intensity. Boosting harder pushes the violets
    // past 1.0, where they clip to white and the palette stops reading.
    gl_FragColor = vec4(col * (0.75 + 0.35 * min(glow, 1.2)), alpha);
}
