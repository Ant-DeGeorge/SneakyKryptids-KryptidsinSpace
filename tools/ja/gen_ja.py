#!/usr/bin/env python3
"""Japanese build: writes build/ja_src/ (a copy of src/*.c with the text in Japanese) for `make japanese`.

src/ is never changed, so the English ROM stays byte for byte the same.
  - Kana glyphs come from the Misaki 8x8 font (tools/ja/misaki.json), drawn in the game's font style.
  - A Japanese string is bytes: ASCII 0x20-0x7E use the normal font; glyph codes are 0x80-0xFF, then
    0x01-0x1F (not 0x0A). src/ja.c (copied in) keeps the glyphs in a small VRAM cache.
  - Each copied source gets a few patches (hooks into txt_tile/txt_put and the comm typewriter, and a
    cache reset after every font load). Each patch must match exactly once, so a change in src/ that
    breaks one stops the build instead of silently missing it.
Run from the repo root: python3 tools/ja/gen_ja.py
"""
import os, re, sys, json, glob, shutil, importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(ROOT, "src")
OUT = os.path.join(ROOT, "build", "ja_src")
sys.path.insert(0, HERE)
from text_code import JA, DIGIT_AT
import text_script

FONT_CHARS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?.-x',:/&+()$"   # tools/gen_assets.py CHARS
CODES = list(range(0x80, 0x100)) + [c for c in range(0x01, 0x20) if c != 0x0A]
# literals the full Japanese build never draws (demo/test builds only)
NOT_SHOWN = {"GUN CAM DEMO V1.0", "NO GUNS DEMO V1.0", "ON FOOT DEMO V1.0", "UNLOCKED TEST BUILD", "FULL GAME: GITHUB"}
GENERATED = {"script_comm.c", "script_scenes.c", "script_data.c"}

# ------------------------------------------------------------------ glyphs
misaki = json.load(open(os.path.join(HERE, "misaki.json"), encoding="utf-8"))
code_of = {}

def code(ch):
    if ch not in code_of:
        assert ch in misaki, f"no Misaki glyph for {ch!r}"
        assert len(code_of) < len(CODES), "more than %d distinct Japanese glyphs" % len(CODES)
        code_of[ch] = CODES[len(code_of)]
    return code_of[ch]

def enc(s):
    """unicode -> C string literal body (Japanese as octal escapes)"""
    o = []
    for ch in s:
        if ch == "\n": o.append("\\n")
        elif ord(ch) < 0x80:
            assert ch in FONT_CHARS or ch.lower() in FONT_CHARS.lower(), f"no font glyph for {ch!r} in {s!r}"
            o.append("\\" + ch if ch in '"\\' else ch)
        else: o.append("\\%03o" % code(ch))
    return '"' + "".join(o) + '"'

def glyph_tile(ch):
    rows = [0] + misaki[ch]   # one row down: the comm panel hides the top pixel row (dakuten live there)
    on = {(x, y) for y, r in enumerate(rows) for x in range(8) if r & (0x80 >> x)}
    px = [[1] * 8 for _ in range(8)]                         # background colour 1
    for (x, y) in on:
        if (x + 1, y + 1) not in on and x < 7 and y < 7: px[y + 1][x + 1] = 0   # shadow, as gen_assets font_panel
    for (x, y) in on: px[y][x] = 3
    b = []
    for r in px:
        b.append(sum((v & 1) << (7 - x) for x, v in enumerate(r)))
        b.append(sum(((v >> 1) & 1) << (7 - x) for x, v in enumerate(r)))
    return b

# ------------------------------------------------------------------ code strings
def width_ok(en, ja):
    el, jl = en.split("\\n"), ja.split("\\n")
    if len(el) == 1: return len(ja) <= len(en)
    return len(jl) <= len(el) and max(map(len, jl)) <= max(map(len, el))

BAD = []
LIT = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
TAGFILES = {"stage.c", "cockpit.c", "escape.c", "ground.c", "arcx.c"}   # HUD messages overwrite the last one in place
def ja_literal(en, fname=""):
    ja = JA[en]
    if fname in TAGFILES and len(en) in (7, 15) and "\\n" not in en and len(ja) < len(en):
        ja = ja + " " * (len(en) - len(ja))
    if not width_ok(en, ja): BAD.append(f"too long: {en!r} -> {ja!r}")
    if "\\n" not in en and en.endswith(" ") and len(ja) < len(en):
        ja = ja + " " * (len(en) - len(ja))                  # pop-up/tag strings overwrite the last one
    if en in DIGIT_AT: assert ja[DIGIT_AT[en]] == "1", (en, ja)
    return enc(ja.replace("\\n", "\n"))

# a patch: (file, exact text, replacement); each must match exactly once
PATCHES = [
    ("core.c", "uint8_t txt_tile(char c) {\n    uint8_t t, b = _current_bank;\n",
     "uint8_t ja_tile(uint8_t c) BANKED;\nextern uint8_t ja_allow1, ja_vb, ja_win;\n"
     "uint8_t txt_tile(char c) {\n    uint8_t t, b = _current_bank;\n"
     "    if ((uint8_t)c >= 0x80 || ((uint8_t)c && (uint8_t)c < 0x20) || (c >= 'A' && c <= 'Z')) return ja_tile((uint8_t)c);\n"),
    ("core.c", "    uint8_t n = 0, t;\n    while (s[n] && n < 20) {\n        t = txt_tile(s[n]);\n"
               "        if (win) { set_win_tile_xy(x + n, y, t); VBK_REG = 1; set_win_tile_xy(x + n, y, attr); VBK_REG = 0; }\n"
               "        else { set_bkg_tile_xy(x + n, y, t); VBK_REG = 1; set_bkg_tile_xy(x + n, y, attr); VBK_REG = 0; }\n",
     "    uint8_t n = 0, t, a;\n    if (win) ja_win = 1;\n    while (s[n] && n < 20) {\n        ja_allow1 = 1; ja_vb = 0; t = txt_tile(s[n]); ja_allow1 = 0;\n"
     "        a = ja_vb ? (attr | 0x08) : attr;\n"
     "        if (win) { set_win_tile_xy(x + n, y, t); VBK_REG = 1; set_win_tile_xy(x + n, y, a); VBK_REG = 0; }\n"
     "        else { set_bkg_tile_xy(x + n, y, t); VBK_REG = 1; set_bkg_tile_xy(x + n, y, a); VBK_REG = 0; }\n"),
    ("objs.c", "            set_win_tile_xy(5 + comm_cx, 3 + comm_cy, txt_tile(c));\n",
     "            { uint8_t t; ja_win = 1; ja_allow1 = 1; ja_vb = 0; t = txt_tile(c); ja_allow1 = 0;\n"
     "              set_win_tile_xy(5 + comm_cx, 3 + comm_cy, t);\n"
     "              VBK_REG = 1; set_win_tile_xy(5 + comm_cx, 3 + comm_cy, ja_vb ? (5 | 0x80 | 0x08) : (5 | 0x80)); VBK_REG = 0; }\n"),
    ("objs.c", "static char comm_buf[48];\n", "static char comm_buf[48];\nextern uint8_t ja_allow1, ja_vb, ja_win;\n"),
    ("objs.c", "    ld_bkg_data(SD->dbank, 1, BOSS_BASE, SD->n_btiles, SD->btiles);\n",
     "    ld_bkg_data(SD->dbank, 1, BOSS_BASE, SD->n_btiles, SD->btiles); ja_boss1(BOSS_BASE, SD->n_btiles, SD->n_scen1);\n"),
    ("objs.c", "extern uint8_t ja_allow1, ja_vb, ja_win;\n", "extern uint8_t ja_allow1, ja_vb, ja_win;\nvoid ja_boss1(uint8_t first, uint8_t n, uint8_t scen1) BANKED;\n"),
    ("scenes.c", "c[8] = '1' + sid;", "c[%d] = '1' + sid;" % DIGIT_AT["MISSION 1 COMPLETE"]),
]
for f in ("arcade.c", "cockpit.c", "ending.c", "stage.c", "armory.c", "escape.c", "ground.c", "scenes.c"):
    PATCHES.append((f, "    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles);\n",
                    "    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles); ja_reset();\n"))

def transform(name, text):
    for f, a, b in PATCHES:
        if f != name: continue
        n = text.count(a)
        assert n == 1, f"patch for {f} matched {n} times: {a[:60]!r}"
        text = text.replace(a, b, 1)
    if "ja_reset();" in text:   # declare it after the last #include
        lines = text.split("\n")
        k = max(i for i, l in enumerate(lines) if l.startswith("#include"))
        lines.insert(k + 1, "void ja_reset(void) BANKED;")
        text = "\n".join(lines)
    if name == "ground.c":   # the OFF THE HULL tag is recognised by its first letter
        a = "tag_s[0] == 'O'"
        assert text.count(a) == 1
        text = text.replace(a, "tag_s[0] == '\\%03o'" % code(JA["OFF THE HULL!  "][0]))
    used = set()
    def rep(m):
        en = m.group(1)
        if en in JA:
            used.add(en)
            return ja_literal(en, name)
        return m.group(0)
    out = []
    for line in text.split("\n"):
        s = line.lstrip()
        out.append(line if s.startswith("#include") or s.startswith("//") else LIT.sub(rep, line))
    return "\n".join(out), used

# ------------------------------------------------------------------ story script
KINSOKU = set("。、!?ー…・ゃゅょっぁぃぅぇぉャュョッァィゥェォ.,)")

def jwrap(txt, w, rows):
    # tokens: words split at spaces, and after 。、!? (no space is shown there when they share a line)
    toks = []
    for k, word in enumerate(txt.split(" ")):
        parts = re.split(r"(?<=[。、!?])(?=[^。、!?.])", word)
        for n, p in enumerate(parts): toks.append((" " if k and not n else "", p))
    lines, cur = [], ""
    for sep, word in toks:
        while word:
            add = (sep if cur else "") + word
            if len(cur) + len(add) <= w:
                cur += add; word = ""
            elif not cur and len(word) > w:               # a word wider than a line: break it, but not before 。etc.
                k = w
                while k > 1 and word[k] in KINSOKU: k -= 1
                lines.append(word[:k]); word = word[k:]
            else:
                lines.append(cur); cur = ""
    if cur: lines.append(cur)
    assert len(lines) <= rows, (txt, lines)
    for l in lines: assert len(l) <= w, (txt, l)
    return lines

def script():
    spec = importlib.util.spec_from_file_location("gen_script", os.path.join(ROOT, "tools", "gen_script.py"))
    g = importlib.util.module_from_spec(spec); spec.loader.exec_module(g)
    T = text_script.S
    g.INTRO = [[(sc, who, ex, T[f"I{s}.{i}"]) for i, (sc, who, ex, _) in enumerate(st)] for s, st in enumerate(g.INTRO)]
    g.OUTRO = [[[(sc, who, ex, T[f"O{s}.{o}.{i}"]) for i, (sc, who, ex, _) in enumerate(lines)]
                for o, lines in enumerate(st)] for s, st in enumerate(g.OUTRO)]
    g.COMMS = [(cid, who, T[cid]) for cid, who, _ in g.COMMS]
    g.NAMES = dict(text_script.NAMES)
    g.OUT = OUT
    g.wrap = jwrap
    g.cstr = lambda s: enc(s)
    g.emit()
    WRITTEN.update(GENERATED)
    os.remove(os.path.join(OUT, "script_ids.h"))           # same ids as src/script_ids.h

# ------------------------------------------------------------------ main
WRITTEN = set()
def put(path, text):
    """write only when the content changed (keeps make from recompiling everything)"""
    WRITTEN.add(os.path.basename(path))
    if os.path.exists(path) and open(path, encoding="latin-1").read() == text: return
    open(path, "w", encoding="latin-1").write(text)

def main():
    os.makedirs(OUT, exist_ok=True)
    used = set()
    for path in sorted(glob.glob(os.path.join(SRC, "*.c"))):
        name = os.path.basename(path)
        if name in GENERATED: continue
        text = open(path, encoding="latin-1").read()
        new, u = transform(name, text)
        used |= u
        if new != text: put(os.path.join(OUT, name), new)
    assert not BAD, "\n".join(BAD)
    missing = [k for k in JA if k not in used]
    assert not missing, f"translations with no matching literal: {missing}"
    script()
    # which font glyphs must stay: every ASCII character left in the Japanese build's text
    keep = set(" 0123456789x?ATSH")       # HUD: digits, space, x, A (boost), T/S/H (cockpit weapon); '?' = cache overflow
    for path in glob.glob(os.path.join(OUT, "*.c")) + [p for p in glob.glob(os.path.join(SRC, "*.c"))
                                                       if not os.path.exists(os.path.join(OUT, os.path.basename(p)))]:
        if os.path.basename(path).startswith("assets_b"): continue
        t = open(path, encoding="latin-1").read()
        t = re.sub(r"(?m)^\s*#include.*$|//.*$", "", t)
        for m in LIT.finditer(t):
            if m.group(1) in NOT_SHOWN: continue
            s = re.sub(r"\\[0-7]{3}|\\n", "", m.group(1)).replace('\\"', '"').replace("\\\\", "\\")
            keep |= {c.upper() for c in s}
    keep &= set(FONT_CHARS)
    keep -= set("ABCDEFGHIJKLMNOPQRSTUVWXYZ")   # capitals go through the cache too (src ja.c)
    free0 = [i for i, c in enumerate(FONT_CHARS) if c not in keep]
    free1 = list(range(224, 230)) + list(range(234, 246)) + list(range(250, 256))   # VRAM bank 1, unused in every mode
    glyphs = sorted(code_of, key=lambda c: CODES.index(code_of[c]))
    map0 = [0xFF] * 64; map1 = [0xFF] * 32
    for i, t in enumerate(free0): map0[t] = i
    for i, t in enumerate(free1): map1[t - 224] = len(free0) + i
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    import gbart
    def latin_tile(ch):      # tools/gen_assets.py font_panel
        px = [[3 if v == 3 else (0 if v == 1 else 1) for v in row] for row in gbart.font_tile(ch)]
        b = []
        for r in px:
            b.append(sum((v & 1) << (7 - x) for x, v in enumerate(r)))
            b.append(sum(((v >> 1) & 1) << (7 - x) for x, v in enumerate(r)))
        return b
    L = ["// generated by tools/ja/gen_ja.py: Misaki 8x8 kana (Num Kadoma, free licence) in the game's font style",
         f"#define JA_LATIN0 {len(glyphs)}",
         f"#define JA_NGLYPH {len(glyphs) + 26}", f"#define JA_NFREE0 {len(free0)}", f"#define JA_NFREE1 {len(free1)}",
         f"#define JA_QMARK {FONT_CHARS.index('?')}",
         "static const uint8_t ja_font[] = {"]
    for ch in glyphs: L.append("    " + ",".join("0x%02X" % b for b in glyph_tile(ch)) + ",  // " + ch)
    for ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZ": L.append("    " + ",".join("0x%02X" % b for b in latin_tile(ch)) + ",  // " + ch)
    L += ["};",
          "static const uint8_t ja_free0[] = {" + ",".join(map(str, free0)) + "};",
          "static const uint8_t ja_free1x[] = {" + ",".join(str(t - 224) for t in free1) + "};",
          "static const uint8_t ja_map0[64] = {" + ",".join(map(str, map0)) + "};"
          ]
    put(os.path.join(OUT, "ja_font.inc"), ("\n".join(L) + "\n").encode("utf-8").decode("latin-1"))
    put(os.path.join(OUT, "ja.c"), open(os.path.join(HERE, "ja.c"), encoding="latin-1").read())
    for f in os.listdir(OUT):    # a source that no longer needs changes falls back to src/
        if f.endswith(".c") and f not in WRITTEN: os.remove(os.path.join(OUT, f))
    json.dump({c: code_of[c] for c in glyphs}, open(os.path.join(OUT, "ja_codes.json"), "w"), ensure_ascii=False)
    print(f"ja: {len(glyphs)} glyphs, {len(free0)}+{len(free1)} cache slots, static font: {''.join(sorted(keep))!r}")

if __name__ == "__main__":
    main()
