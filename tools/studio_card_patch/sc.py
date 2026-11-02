import re,sys
from PIL import Image
import os; D=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','src','studio_card')+'/'
def arrs(path):
    s=open(path).read(); out={}
    for m in re.finditer(r'(?:static )?const (?:u?int\d+_t) (\w+)\[[^\]]*\]=\{(.*?)\};',s,re.S):
        body=re.sub(r'//.*','',m.group(2))
        out[m.group(1)]=[int(x,0) for x in re.findall(r'-?0x[0-9a-fA-F]+|-?\d+',body)]
    return out
A=arrs(D+'studio_card_data.h'); G=arrs(D+'studio_card_gfx.h')
def tile(bank,t):
    d=(G['sc_tiles1'] if bank else G['sc_tiles0'])[t*16:t*16+16]
    return [[((d[2*r]>>(7-c))&1)|(((d[2*r+1]>>(7-c))&1)<<1) for c in range(8)] for r in range(8)]
def rgb(c): return ((c&31)*8,((c>>5)&31)*8,((c>>10)&31)*8)
def render(f=329,stripes=True):
    pl=A['sc_pal_logic'];rg=A['sc_rgb']
    eye=A['sc_eye_tab'][A['sc_f_eyeId'][f]*14:][:14]; ri=A['sc_f_ring'][f]
    ring=A['sc_ring_tab'][ri*8:][:8] if ri!=255 else None
    im=Image.new('RGB',(160,144))
    for y in range(18):
      for x in range(20):
        i=y*20+x;k=A['sc_pos_kind'][i];t=A['sc_stripe'][A['sc_mod3'][y]];a=2
        if k:
            a=A['sc_pos_attr'][i]
            t=A['sc_pos_tile'][i] if k==1 else eye[A['sc_pos_tile'][i]] if k==2 else (ring[A['sc_pos_tile'][i]] if ring else t)
            if k==3 and not ring: a=2
        p=a&7;b=(a>>3)&1;px=tile(b,t)
        for r in range(8):
          for c in range(8):
            v=px[r][c]
            if a&0x20: v=px[r][7-c]
            if a&0x40: v=px[7-r][c] if not a&0x20 else px[7-r][7-c]
            im.putpixel((x*8+c,y*8+r),rgb(rg[pl[p*4+v]]))
    return im
if __name__=='__main__':
    im=render(); im.resize((480,432),0).save('/tmp/r.png')
    for y in range(9):
        print(' '.join('%d:%02x%s'%(A['sc_pos_kind'][y*20+x],A['sc_pos_attr'][y*20+x],'') for x in range(20)))
