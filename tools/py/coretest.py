"""Boot test and rollback check for mka_fbneo.dll.

For each set: load it from the ROM folder, run its attract mode, save PNG frames, then check what rollback needs:
save a state, run frames with scripted inputs, load the state, run the same frames again, and compare the pictures
and the states the two runs end with. Prints the state size and how fast frames run with and without drawing.

  python tools/py/coretest.py --roms C:/path/to/roms --out <folder> umk3 mk2 mk3 mk
"""
import argparse
import ctypes
import hashlib
import os
import struct
import sys
import time
import zlib

BUTTONS = ["up", "down", "left", "right", "b1", "b2", "b3", "b4", "b5", "b6", "start", "coin", "service", "configure"]
B = {name: i for i, name in enumerate(BUTTONS)}


def load_core(path):
    dll = ctypes.CDLL(path)
    sig = {
        "FBHeadlessInit": ([], ctypes.c_int),
        "FBHeadlessExit": ([], None),
        "FBHeadlessSetRomPath": ([ctypes.c_char_p], None),
        "FBHeadlessLoadDriver": ([ctypes.c_char_p], ctypes.c_int),
        "FBHeadlessRunFrame": ([ctypes.c_int], None),
        "FBHeadlessGetVideoBuffer": ([], ctypes.c_void_p),
        "FBHeadlessGetVideoWidth": ([], ctypes.c_int),
        "FBHeadlessGetVideoHeight": ([], ctypes.c_int),
        "FBHeadlessGetVideoPitch": ([], ctypes.c_int),
        "FBHeadlessGetVideoBPP": ([], ctypes.c_int),
        "FBHeadlessGetAudioSampleRate": ([], ctypes.c_int),
        "FBHeadlessGetAudioBuffer": ([], ctypes.c_void_p),
        "FBHeadlessGetAudioFrames": ([], ctypes.c_int),
        "FBHeadlessGetFrameRate": ([], ctypes.c_int),
        "FBHeadlessSetInput": ([ctypes.c_int, ctypes.c_int, ctypes.c_int], None),
        "FBHeadlessGetMemoryRegionCount": ([], ctypes.c_int),
        "FBHeadlessGetMemoryRegion": ([ctypes.c_int, ctypes.POINTER(ctypes.c_uint), ctypes.POINTER(ctypes.c_uint),
                                       ctypes.POINTER(ctypes.c_int)], ctypes.c_int),
        "MKA_StateSize": ([], ctypes.c_uint),
        "MKA_StateSave": ([ctypes.c_void_p, ctypes.c_uint], ctypes.c_uint),
        "MKA_StateLoad": ([ctypes.c_void_p, ctypes.c_uint], ctypes.c_int),
        "MKA_StateAreas": ([], ctypes.c_uint),
        "MKA_SetPaths": ([ctypes.c_char_p, ctypes.c_char_p], None),
        "MKA_DriverCount": ([], ctypes.c_int),
        "MKA_DriverText": ([ctypes.c_int, ctypes.c_int], ctypes.c_char_p),
    }
    for name, (args, res) in sig.items():
        f = getattr(dll, name)
        f.argtypes = args
        f.restype = res
    return dll


def frame_bytes(core):
    w, h, pitch = core.FBHeadlessGetVideoWidth(), core.FBHeadlessGetVideoHeight(), core.FBHeadlessGetVideoPitch()
    buf = core.FBHeadlessGetVideoBuffer()
    return w, h, pitch, ctypes.string_at(buf, pitch * h)


def write_png(path, w, h, pitch, data):
    # 32bpp XRGB8888 (little-endian B G R X) to an RGB PNG, no third-party modules
    rows = bytearray()
    for y in range(h):
        row = data[y * pitch:y * pitch + w * 4]
        rows.append(0)
        rgb = bytearray(w * 3)
        rgb[0::3] = row[2::4]
        rgb[1::3] = row[1::4]
        rgb[2::3] = row[0::4]
        rows += rgb

    def chunk(kind, payload):
        c = struct.pack(">I", len(payload)) + kind + payload
        return c + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(rows), 6)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def set_inputs(core, held):
    for p in range(2):
        for b in range(len(BUTTONS)):
            core.FBHeadlessSetInput(p, b, 1 if (p, b) in held else 0)


def scripted(frame):
    """A busy, repeatable input pattern for both players: coins, starts, walking and buttons."""
    held = set()
    if frame % 240 in range(0, 4):
        held |= {(0, B["coin"]), (1, B["coin"])}
    if frame % 240 in range(30, 34):
        held |= {(0, B["start"]), (1, B["start"])}
    for p in range(2):
        phase = (frame // (7 + p * 3)) % 6
        held.add((p, [B["left"], B["right"], B["up"], B["down"], B["right"], B["left"]][phase]))
        if (frame + p * 5) % 11 < 3:
            held.add((p, B["b1"] + (frame // 11 + p) % 5))
    return held


def run(core, frames, start, draw=0):
    for i in range(frames):
        set_inputs(core, scripted(start + i))
        core.FBHeadlessRunFrame(draw)


def state(core):
    n = core.MKA_StateSize()
    buf = ctypes.create_string_buffer(n)
    got = core.MKA_StateSave(buf, n)
    return buf.raw[:got]


def test_set(core, name, out, boot_frames, check_frames):
    t0 = time.perf_counter()
    if core.FBHeadlessLoadDriver(name.encode()):
        print(f"{name}: FAILED to load (ROMs missing or wrong?)")
        return False
    fps = core.FBHeadlessGetFrameRate() / 100.0
    w, h = core.FBHeadlessGetVideoWidth(), core.FBHeadlessGetVideoHeight()
    print(f"{name}: loaded in {time.perf_counter() - t0:.2f} s, {w}x{h} at {fps:.4f} Hz, "
          f"audio {core.FBHeadlessGetAudioSampleRate()} Hz")

    # attract mode, a picture every so often
    for i in range(boot_frames):
        set_inputs(core, set())
        core.FBHeadlessRunFrame(1)
        if (i + 1) in (boot_frames // 3, 2 * boot_frames // 3, boot_frames):
            write_png(os.path.join(out, f"{name}-{i + 1:05d}.png"), *frame_bytes(core))

    # speed: frames with and without drawing (rollback runs frames again without drawing)
    t = time.perf_counter()
    run(core, 300, 0, draw=1)
    drawn = 300 / (time.perf_counter() - t)
    t = time.perf_counter()
    run(core, 300, 300, draw=0)
    undrawn = 300 / (time.perf_counter() - t)

    # rollback check: from a saved state, the same inputs must give the same pictures and the same end state
    start = state(core)
    t = time.perf_counter()
    for _ in range(50):
        state(core)
    save_ms = (time.perf_counter() - t) * 1000 / 50
    runs = []
    for attempt in range(2):
        if core.MKA_StateLoad(start, len(start)):
            print(f"{name}: state load FAILED")
            return False
        pics = hashlib.sha1()
        for i in range(check_frames):
            set_inputs(core, scripted(10000 + i))
            core.FBHeadlessRunFrame(1)
            pics.update(frame_bytes(core)[3])
        runs.append((pics.hexdigest(), hashlib.sha1(state(core)).hexdigest()))
    t = time.perf_counter()
    for _ in range(50):
        core.MKA_StateLoad(start, len(start))
    load_ms = (time.perf_counter() - t) * 1000 / 50
    write_png(os.path.join(out, f"{name}-played.png"), *frame_bytes(core))

    same = runs[0] == runs[1]
    print(f"{name}: state {len(start) / 1024:.0f} KB in {core.MKA_StateAreas()} areas, save {save_ms:.2f} ms, "
          f"load {load_ms:.2f} ms; {drawn:.0f} frames/s drawn, {undrawn:.0f} undrawn")
    print(f"{name}: replay of {check_frames} frames from a saved state: "
          f"{'IDENTICAL' if same else 'DIFFERENT'} (pictures {runs[0][0][:12]} / {runs[1][0][:12]}, "
          f"states {runs[0][1][:12]} / {runs[1][1][:12]})")
    return same


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--core", default=os.path.join(os.path.dirname(__file__), "..", "..", "build", "Release", "core",
                                                   "mka_fbneo.dll"))
    ap.add_argument("--roms", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--boot", type=int, default=1200, help="attract-mode frames before the checks")
    ap.add_argument("--check", type=int, default=600, help="frames replayed from a saved state")
    ap.add_argument("--list", action="store_true", help="print the sets the core knows and exit")
    ap.add_argument("sets", nargs="*")
    a = ap.parse_args()

    os.makedirs(a.out, exist_ok=True)
    core = load_core(os.path.abspath(a.core))
    core.FBHeadlessInit()
    nv = os.path.join(a.out, "nvram")
    os.makedirs(nv, exist_ok=True)
    core.MKA_SetPaths(nv.encode(), os.path.join(a.out, "support").encode())
    if a.list:
        for i in range(core.MKA_DriverCount()):
            n, full, parent = (core.MKA_DriverText(i, k).decode() for k in (0, 1, 2))
            print(f"{n:12s} {parent:10s} {full}")
        return 0
    roms = a.roms.replace("\\", "/")
    core.FBHeadlessSetRomPath((roms if roms.endswith("/") else roms + "/").encode())
    ok = True
    for s in a.sets:
        ok &= test_set(core, s, a.out, a.boot, a.check)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
