"""astro_ref.py [PICOMITE_TREE]: astro.expected, the expected output of astro.bas, from the PicoMite's own astronomy (its
io/GPS.c: the catalogue, getSiderealTime, localRA, getCelestialPosition, applyProperMotionAndPrecession,
getJulianCenturies, findStarInCatalog) compiled on a PC with gcc (in WSL), for the place, time and bodies astro.bas
uses; its command lines' messages and SLEW's arithmetic as the PicoMite's cmd_star, cmd_locate and cmd_slew have them.
It writes astro.expected beside itself."""
import os, re, subprocess, sys, tempfile

PM = sys.argv[1] if len(sys.argv) > 1 else r'D:\Dropbox\PicoMite\PicoMite'
HERE = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(PM, 'io', 'GPS.c'), encoding='utf-8').read().replace('\r\n', '\n').split('\n')


def find(pat, start=0):
    for i in range(start, len(src)):
        if re.search(pat, src[i]):
            return i
    sys.exit('not found: ' + pat)


# the types and catalogue (from the enum to the prototypes), and the maths (from getSiderealTime's comment block to
# the end of findStarInCatalog)
a = find(r'^enum CelestialBody')
b = find(r'^void localRA\(struct s_RADec.*;$', a)             # (the last prototype)
c = find(r'^MMFLOAT getSiderealTime\([^;]*$')                    # (the definition)
while not src[c - 1].startswith('#ifdef'):
    c -= 1
d = find(r'^static int findStarInCatalog')
d = find(r'^}', d)
code = '\n'.join(src[a - 2:b + 1] + src[c:d + 1])
code = code.replace('(unsigned char *)', '')

main = r'''
static double lat, lon, T, sid;
static void place(int dd, int mo, int yy, int hh, int mi, int ss, double la, double lo) {
    lat = la; lon = lo;
    T = getJulianCenturies(dd, mo, yy, hh, mi, ss);
    sid = getSiderealTime(dd, mo, yy, hh, mi, ss, lo);
    printf("lst %.4f\n", sid);
}
static void show(const char *label, struct s_RADec s, int radec) {
    double alt, az;
    localRA(s, &alt, &az, lat, sid);
    printf("%s %.4f %.4f", label, alt, az);
    if (radec) printf(" %.4f %.4f", s.RA, s.Dec);
    printf("\n");
}
static struct s_RADec body(const char *name) {
    static const char *planets[] = {"MOON", "MERCURY", "VENUS", "MARS", "JUPITER", "SATURN", "URANUS", "NEPTUNE", "SUN"};
    for (int i = 0; i < 9; i++)
        if (strcasecmp(name, planets[i]) == 0) return getCelestialPosition((enum CelestialBody)i, T, lat, sid);
    int k = findStarInCatalog(name);
    if (k < 0) { printf("no star %s\n", name); return (struct s_RADec){0, 0}; }
    return applyProperMotionAndPrecession(starCatalog[k].ra_deg / 15.0, starCatalog[k].dec_deg, starCatalog[k].pm_ra,
                                          starCatalog[k].pm_dec, T);
}
int main(void) {
    const char *b[] = {"SUN", "MOON", "MERCURY", "VENUS", "MARS", "JUPITER", "SATURN", "URANUS", "NEPTUNE", "Vega",
                       "Sirius", "Polaris", "Andromeda Galaxy", "Rigil Kent"};
    place(21, 10, 2026, 20, 0, 0, 51.4769, -0.0005);
    for (int i = 0; i < 14; i++) show(b[i], body(b[i]), 1);
    show("MOON", body("MOON"), 0);
    show("Vega", body("Vega"), 0);
    show("Andromeda Galaxy", body("Andromeda Galaxy"), 1);
    show("s$", body("Capella"), 0);
    show("s$ space", body("Capella"), 0);
    show("MID$", body("Polaris"), 0);
    show("AS STRING", body("Deneb"), 0);
    show("vega", body("vega"), 0);
    show("manual", applyProperMotionAndPrecession(18.6156, 38.7837, 0.0, 0.0, T), 0);
    show("manual pm", applyProperMotionAndPrecession(18.6156, 38.7837, 0.2, 0.28, T), 0);
    show("manual out", applyProperMotionAndPrecession(18.6156, 38.7837, 0.2, 0.28, T), 1);
    place(21, 10, 2026, 6, 30, 15, -33.8688, 151.2093);
    show("SUN", body("SUN"), 0);
    place(21, 10, 2026, 6, 30, 15, -33.8688, 151.2093);
    return 0;
}
'''
head = '#include <stdio.h>\n#include <math.h>\n#include <string.h>\n#include <strings.h>\n#include <ctype.h>\n' \
       'typedef double MMFLOAT;\n'
tmp = tempfile.mkdtemp()
open(os.path.join(tmp, 'ref.c'), 'w', encoding='utf-8', newline='\n').write(head + code + main)
wsl = '/mnt/' + tmp[0].lower() + tmp[2:].replace('\\', '/')
r = subprocess.run(['wsl', '-e', 'bash', '-lc', 'cd "%s" && gcc -O2 -o ref ref.c -lm && ./ref' % wsl],
                   capture_output=True, text=True)
if r.returncode:
    sys.exit(r.stdout + r.stderr)
lines = r.stdout.split('\n')
q = '"'
err = [
    'Astro fred alt, az: Unknown body',
    'Astro %sfred%s, alt, az: Unknown body' % (q, q),
    'Astro alt, az: Argument count',
    'Star SUN alt, az: GPS data not valid',
    'Location %s32/10/2026 20:00:00%s, 0, 0: Invalid date' % (q, q),
    'Location %s21/10/2026 25:00:00%s, 0, 0: Invalid time' % (q, q),
    'Location %s21/10/2026 20:00:00%s, 95, 0: Invalid latitude' % (q, q),
    'Location %s21/10/2026 20:00:00%s, 0, 190: Invalid longitude' % (q, q),
]
slew = [
    # mount (2 h, 10 deg) west of the meridian (LST 3 h), the target (4 h, 20 deg) east: to the meridian, the flip
    # (from the west: -12 h, -180 deg), then on
    'slew 1.0000 10.0000 -12.0000 -180.0000 1.0000 0.0000',
    # the target (2.5 h, 20 deg) west too (LST 5 h): one slew
    'slew 0.5000 10.0000 0.0000 0.0000 0.0000 0.0000',
    'Slew d1, d2, f1, f2, d3, d4, 25, 10, 4, 20, 3: RA must be 0-24 hours',
    'Slew d1, d2, f1, f2, d3, d4, 2, 10, 4, 95, 3: Declination must be -90 to +90 degrees',
    'Slew d1, d2, f1, f2, d3, d4, 2, 10, 4, 20, 24: LST must be 0-24 hours',
]
out = [l for l in lines if l] + err + slew
open(os.path.join(HERE, 'astro.expected'), 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
print('\n'.join(out))
