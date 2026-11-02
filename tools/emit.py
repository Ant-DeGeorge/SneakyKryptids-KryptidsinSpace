"""C emitter for banked asset data (shared by all generators)."""
import os
from paint import hx

def rgb555(c):
    if isinstance(c, str):
        c = hx(c)
    r, g, b = (min(31, int(round(v / 255 * 31))) for v in c)
    return r | (g << 5) | (b << 10)

def pal_words(cols):
    return [rgb555(c) for c in cols]


class Out:
    def __init__(self, src_dir, header="assets.h", prefix="assets_b"):
        self.src = src_dir
        self.header = header
        self.prefix = prefix
        self.h = []
        self.c = {}
        self.sizes = {}

    def carray(self, bank, name, data, typ="uint8_t"):
        data = list(data) or [0]
        self.h.append(f"extern const {typ} {name}[];  // bank {bank}, {len(data)}")
        if typ == "uint16_t":
            body = ",".join(f"0x{v:04X}" for v in data)
        else:
            body = ",".join(str(int(v)) for v in data)
        self.c.setdefault(bank, []).append(f"const {typ} {name}[] = {{{body}}};")
        self.sizes[bank] = self.sizes.get(bank, 0) + len(data) * (2 if typ == "uint16_t" else 1)

    def define(self, name, val):
        self.h.append(f"#define {name} {val}")

    def raw_h(self, line):
        self.h.append(line)

    def write(self, guard="ASSETS_H"):
        hdr = ["// generated - do not edit", f"#ifndef {guard}", f"#define {guard}", "#include <stdint.h>"] + self.h + ["#endif"]
        open(os.path.join(self.src, self.header), "w").write("\n".join(hdr) + "\n")
        for bank, lines in self.c.items():
            fn = os.path.join(self.src, f"{self.prefix}{bank}.c")
            pragma = f"#pragma bank {bank}\n" if bank else ""
            open(fn, "w").write(f"// generated - do not edit\n{pragma}#include <stdint.h>\n" + "\n".join(lines) + "\n")
        for b, s in self.sizes.items():
            assert b == 0 or s <= 16384, f"bank {b} overflow: {s} bytes"
        return self.sizes
