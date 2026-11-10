#!/usr/bin/env python3
"""Render songs to WAV/MP3 through SameBoy (accurate Game Boy Color APU) and the sound-test ROM.

  python3 tools/music/render.py OUTDIR [SONG ...]      (default: all songs; needs `make jukebox`)
Looping songs play once through, then the loop starts again for a few seconds and fades out.
"""
import os, sys, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')
sys.path.insert(0, HERE)
import uge
from compose import ORDER

ROM = os.path.join(ROOT, 'build', 'jukebox.gbc')
HARNESS = os.path.join(ROOT, 'test', 'sameboy', 'sb_harness')


def length_frames(song):
    """Frames for one pass, and whether the song loops (a silent end means a jingle)."""
    n_orders = max(len(o) for o in song.orders)
    jump = None
    for c in range(4):
        last = song.patterns[song.orders[c][-1]][63]
        if last.fx == 0xB: jump = last.param
    silent_end = jump == n_orders                   # Bxx is 1-based: the silent last order jumps to itself
    played = (n_orders - 1) if silent_end else n_orders
    return played * 64 * song.ticks_per_row, not silent_end


def sym(name):
    for l in open(ROM[:-4] + '.noi'):
        p = l.split()
        if len(p) == 3 and p[1] == name: return int(p[2], 16)
    raise KeyError(name)


def render(key, outdir):
    song = uge.Song.load(os.path.join(ROOT, 'music', key.lower() + '.uge'))
    frames, loops = length_frames(song)
    tail = 300 if loops else 90
    wav = os.path.join(outdir, key.lower() + '.raw.wav')
    script = os.path.join(outdir, key.lower() + '.sb.txt')
    with open(script, 'w') as f:
        f.write(f'30 poke {sym("_jb_req"):x}={ORDER.index(key):x}\n31 wav {wav}\n{31 + frames + tail} end\n')
    subprocess.run([HARNESS, ROM, script, outdir, '.'], cwd=os.path.dirname(HARNESS), check=True,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    secs = (frames + tail) / 59.7275
    base = os.path.join(outdir, key.lower())
    fade = f'afade=t=out:st={secs - 4:.2f}:d=4,' if loops else ''
    import wave, array
    w = wave.open(wav); pcm = array.array('h', w.readframes(w.getnframes())); w.close()
    peak = max(1, max(pcm), -min(pcm)) / 32768
    gain = min(1.6, 0.93 / peak)                        # the GBC mix is quiet: lift it, but never clip
    af = fade + f'volume={gain:.3f}'
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', wav, '-af', af, base + '.wav'], check=True)
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', base + '.wav', '-codec:a', 'libmp3lame', '-b:a', '192k', base + '.mp3'], check=True)
    os.remove(wav); os.remove(script)
    return secs


if __name__ == '__main__':
    out = sys.argv[1]; os.makedirs(out, exist_ok=True)
    for k in [a.upper() for a in sys.argv[2:]] or ORDER:
        print(f'{k}: {render(k, os.path.abspath(out)):.1f} s')
