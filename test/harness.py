"""PyBoy harness: symbol lookup from the GBDK .noi file + helpers."""
import os, re
from pyboy import PyBoy

class Game:
    def __init__(self, rom, speed=0):
        self.p = PyBoy(rom, window="null", cgb=True, sound_emulated=False)
        self.p.set_emulation_speed(speed)
        self.sym = {}
        noi = os.path.splitext(rom)[0] + ".noi"
        for line in open(noi):
            m = re.match(r"DEF (\S+) 0x([0-9A-Fa-f]+)", line)
            if m: self.sym[m.group(1).lstrip('_')] = int(m.group(2), 16)
        self.frame = 0
    def m8(self, name, off=0):
        return self.p.memory[self.sym[name] + off]
    def m16(self, name, off=0):
        a = self.sym[name] + off
        return self.p.memory[a] | (self.p.memory[a + 1] << 8)
    def m32(self, name):
        a = self.sym[name]
        return sum(self.p.memory[a + i] << (8 * i) for i in range(4))
    def s8(self, name, v, off=0):
        self.p.memory[self.sym[name] + off] = v & 0xFF
    def tick(self, n=1):
        for _ in range(n): self.p.tick(); self.frame += 1
    def press(self, b, f=3):
        self.p.button_press(b); self.tick(f); self.p.button_release(b); self.tick(2)
    def hold(self, b): self.p.button_press(b)
    def release(self, b): self.p.button_release(b)
    def shot(self, path, scale=2):
        self.p.screen.image.convert("RGB").resize((160 * scale, 144 * scale), 0).save(path)
