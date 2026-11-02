#!/usr/bin/env python3
"""Smoke test: boots the ROM, walks title -> intro -> stage, saves screenshots."""
import sys, os
from pyboy import PyBoy
rom = sys.argv[1]; out = sys.argv[2]
os.makedirs(out, exist_ok=True)
p = PyBoy(rom, window="null", cgb=True, sound_emulated=False)
p.set_emulation_speed(0)
n = [0]
def tick(k=1):
    for _ in range(k): p.tick()
def shot(name):
    p.screen.image.convert("RGB").resize((320, 288), 0).save(f"{out}/{n[0]:02d}_{name}.png"); n[0] += 1
def press(b, f=3):
    p.button_press(b); tick(f); p.button_release(b); tick(2)
tick(90); shot("title")
press("right"); tick(10); shot("title_hunter"); press("left"); tick(5)
press("a"); tick(60); shot("intro1")
tick(120); shot("intro1b")
press("a"); tick(90); shot("intro2")
press("start"); tick(60); shot("stage_start")
tick(120); shot("stage_2s")
p.button_press("left"); tick(40); p.button_release("left")
tick(300); shot("stage_7s")
tick(400); shot("stage_14s")
tick(600); shot("stage_24s")
p.stop(save=False)
