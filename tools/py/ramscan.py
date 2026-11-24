"""Finds what a game keeps in RAM about a two-player fight (round wins, the match's end) by playing scripted matches.

Boots the set with no battery RAM (pressing BLOCK past the CMOS screen), puts coins in for both players, starts both,
picks the fighters under the cursors, then lets one side walk forward and punch while the other stands still. The
flat RAM (FBNeo's "All RAM", as FBHeadlessReadAchievementMemory shows it) is sampled every few frames, and bytes that
count 0, 1, 2 as the attacker wins rounds (and stay 0 for the other side) are listed. Pictures of the fight go to
--out so the rounds can be checked by eye.

  python tools/py/ramscan.py --roms <rom folder> --out <folder> umk3 --winner 1
"""
import argparse
import ctypes
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import coretest as ct  # noqa: E402

B = ct.B


class Game:
    def __init__(self, core, out):
        self.core = core
        self.out = out
        self.frame = 0
        n = 0x400000
        self.buf = (ctypes.c_ubyte * n)()
        core.FBHeadlessReadAchievementMemory.argtypes = [ctypes.c_uint, ctypes.c_void_p, ctypes.c_uint]
        core.FBHeadlessReadAchievementMemory.restype = ctypes.c_uint
        s, l, be = ctypes.c_uint(), ctypes.c_uint(), ctypes.c_int()
        core.FBHeadlessGetMemoryRegion(0, ctypes.byref(s), ctypes.byref(l), ctypes.byref(be))
        self.ram_len = l.value

    def ram(self):
        got = self.core.FBHeadlessReadAchievementMemory(0, self.buf, self.ram_len)
        return bytes(self.buf[:got])

    def run(self, frames, held=lambda f: set(), draw=0):
        for _ in range(frames):
            ct.set_inputs(self.core, held(self.frame))
            self.core.FBHeadlessRunFrame(draw)
            self.frame += 1

    def shot(self, name):
        self.core.FBHeadlessRunFrame(1)
        self.frame += 1
        ct.write_png(os.path.join(self.out, name), *ct.frame_bytes(self.core))


def boot_and_select(g, boot):
    # past the CMOS screen: BLOCK now and then, then the attract mode
    g.run(boot, lambda f: {(0, B["b2"]), (1, B["b2"])} if 120 <= f <= 1500 and f % 150 < 4 else set())
    g.run(4, lambda f: {(0, B["coin"]), (1, B["coin"])})
    g.run(30)
    g.run(4, lambda f: {(0, B["start"])})
    g.run(240)  # player 1's select screen is up: player 2 joins there
    for _ in range(3):
        g.run(4, lambda f: {(1, B["coin"])})
        g.run(30)
        g.run(4, lambda f: {(1, B["start"])})
        g.run(60)
    g.run(120)
    g.shot("01-select.png")
    g.run(4, lambda f: {(0, B["b1"]), (1, B["b1"])})  # both take the fighter under the cursor
    g.run(60)
    g.run(4, lambda f: {(0, B["b1"]), (1, B["b1"])})  # (a second press, for a select screen that wants one more)
    g.run(60)
    g.shot("02-after-pick.png")


def fight(g, attacker, seconds, sample_every=6):
    """attacker 0 or 1 walks forward and punches; the other stands still. Returns (frame, ram) samples."""
    fwd = B["right"] if attacker == 0 else B["left"]
    samples = []

    def held(f):
        # walk in for most of a second, then three punches; again and again
        t = f % 60
        if t < 36:
            return {(attacker, fwd)}
        if t in range(36, 39) or t in range(44, 47) or t in range(52, 55):
            return {(attacker, B["b1"] if t < 52 else B["b4"])}
        return set()

    total = int(seconds * 54.7)
    for i in range(total):
        ct.set_inputs(g.core, held(g.frame))
        g.core.FBHeadlessRunFrame(0)
        g.frame += 1
        if i % sample_every == 0:
            samples.append((g.frame, g.ram()))
        if i % int(54.7 * 3) == 0:
            g.shot(f"fight-{i // 54:03d}s.png")
    return samples


def counters(samples, max_value=3, min_final=2, reset=False):
    """Bytes that start at 0, never go down, end at min_final..max_value and change at most max_value times.
    reset: a byte may also fall back to 0 once it reached min_final (a game that clears its round wins soon after the
    match), so only its rise counts."""
    first = samples[0][1]
    n = len(first)
    cand = [i for i in range(n) if first[i] == 0]
    for _, ram in samples[1:]:
        cand = [i for i in cand if ram[i] <= max_value]
    hist = {}
    for i in cand:
        seq = [ram[i] for _, ram in samples]
        if reset and max(seq) >= min_final:
            top = seq.index(max(seq))
            seq = seq[:top + 1]
        if seq[-1] < min_final:
            continue
        if any(b < a for a, b in zip(seq, seq[1:])):
            continue
        changes = [(samples[k + 1][0], seq[k + 1]) for k in range(len(seq) - 1) if seq[k + 1] != seq[k]]
        if reset and len(changes) < min_final:
            continue
        if min_final <= len(changes) <= max_value:
            hist[i] = changes
    return hist


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--core", default=os.path.join(os.path.dirname(__file__), "..", "..", "build", "Release", "core",
                                                   "mka_fbneo.dll"))
    ap.add_argument("--roms", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--boot", type=int, default=1700)
    ap.add_argument("--winner", type=int, default=1, help="1 or 2: the side that attacks")
    ap.add_argument("--seconds", type=float, default=75)
    ap.add_argument("--reset", action="store_true", help="counters may fall back to 0 after the match")
    ap.add_argument("set")
    a = ap.parse_args()
    os.makedirs(os.path.join(a.out, "nv"), exist_ok=True)
    core = ct.load_core(os.path.abspath(a.core))
    core.FBHeadlessInit()
    core.MKA_SetPaths(os.path.join(a.out, "nv").encode(), os.path.join(a.out, "sup").encode())
    core.FBHeadlessSetRomPath((a.roms.replace("\\", "/").rstrip("/") + "/").encode())
    if core.FBHeadlessLoadDriver(a.set.encode()):
        print("could not load", a.set)
        return 1
    g = Game(core, a.out)
    boot_and_select(g, a.boot)
    samples = fight(g, a.winner - 1, a.seconds)
    found = counters(samples, reset=a.reset)
    print(f"{a.set}: side {a.winner} attacked for {a.seconds:.0f} s; {len(samples)} samples; "
          f"{len(found)} counting bytes")
    for off, changes in sorted(found.items(), key=lambda kv: (len(kv[1]), kv[0]))[:40]:
        print(f"  RAM+{off:#08x}: " + ", ".join(f"{v} at frame {f}" for f, v in changes))
    # keep the samples' offsets of interest for a second run to compare
    with open(os.path.join(a.out, f"{a.set}-side{a.winner}.txt"), "w") as f:
        for off, changes in sorted(found.items()):
            f.write(f"{off:#08x} " + " ".join(f"{fr}:{v}" for fr, v in changes) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
