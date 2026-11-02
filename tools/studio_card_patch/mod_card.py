import re, sc
from sc import A,G,rgb
names='BG LN CR BE DK TL TD RD RK GR'.split(); C={n:i for i,n in enumerate(names)}
D=sc.D
im=sc.render(329)
inv={rgb(c):i for i,c in enumerate(A['sc_rgb'])}
L=[[inv[im.getpixel((x,y))] for x in range(160)] for y in range(144)]
# ---- tiles
T=[[sc.tile(0,t) if t<len(G["sc_tiles0"])//16 else [[0]*8 for _ in range(8)] for t in range(256)],[sc.tile(1,t) if t<len(G["sc_tiles1"])//16 else [[0]*8 for _ in range(8)] for t in range(256)]]
free={0:list(range(len(G["sc_tiles0"])//16,256)),1:list(range(len(G["sc_tiles1"])//16,256))}
def stripe_ln(y):  # map pixel row y -> is LN stripe
    st=T[0][A['sc_stripe'][A['sc_mod3'][y//8]]]
    return st[y%8][0]==2
pl=A['sc_pal_logic']
# ---- glyphs from old tagline
glyph={}
for s,x0,y0 in (("STORIES THE ALGORITHM COULD NOT APPROVE",3,126),("ANIMATION + COMICS + GAMES - EST. 2026",5,136)):
    for i,ch in enumerate(s):
        g=[[1 if L[y0+r][x0+4*i+c] in (C['GR'],C['TL']) else 0 for c in range(3)] for r in range(5)]
        glyph.setdefault(ch,g)
glyph['J']=[[0,0,1],[0,0,1],[0,0,1],[1,0,1],[0,1,0]]
glyph['/']=[[0,0,1],[0,0,1],[0,1,0],[1,0,0],[1,0,0]]
def text(s,x0,y0,colfn):
    for i,ch in enumerate(s):
        g=glyph[ch]
        for r in range(5):
            for c in range(3):
                if g[r][c]: L[y0+r][x0+4*i+c]=colfn(i)
# ---- 1. palette 0: DK slot -> LN
pl[2]=C['LN']
def cell_pal_tiles():
    pass
# map cells: kind1 positions
kind=A['sc_pos_kind'];attr=A['sc_pos_attr'];ptile=A['sc_pos_tile'];vis=A['sc_pos_vis']
newcache={}
def put_tile(bank,orig,px,y,reuse_pool=None):
    key=(bank,tuple(map(tuple,px)))
    if key in newcache: return newcache[key]
    if orig is not None and orig not in [v for k,v in newcache.items() if k[0]==bank]:
        if T[bank][orig]==px or orig not in claimed[bank]:
            T[bank][orig]=px; claimed[bank].add(orig); newcache[key]=orig; return orig
    pool=reuse_pool if reuse_pool else free[bank]
    t=pool.pop(0); T[bank][t]=px; claimed[bank].add(t); newcache[key]=t; return t
claimed={0:set(),1:set()}
def destripe(px,palidx,y0):
    lnidx=[pl[palidx*4+j] for j in range(4)].index(C['LN'])
    out=[]
    for r in range(8):
        row=[]
        for v in px[r]:
            if palidx==0 and v==2: v=0          # old DK -> BG (near identical)
            if v==0 and stripe_ln(y0+r): v=lnidx
            row.append(v)
        out.append(row)
    return out
# emblem kind1 pal0 cells + eye cells pal0 + ring cells (pal3)
for i in range(360):
    x,y=i%20,i//20
    if kind[i]==1 and (attr[i]&7)==0:
        b=(attr[i]>>3)&1
        px=destripe(sc.tile(b,ptile[i]),0,y*8)
        if all(px[r][c]==(2 if stripe_ln(y*8+r) else 0) for r in range(8) for c in range(8)):
            kind[i]=0; ptile[i]=0; attr[i]=2; vis[i]=0; continue
        ptile[i]=put_tile(b,ptile[i],px,y)
eye=A['sc_eye_tab']
for e in range(len(eye)//14):
    for j in range(14):
        x,y=A['sc_eye_x'][j],A['sc_eye_y'][j]; i=y*20+x
        p=attr[i]&7;b=(attr[i]>>3)&1
        if p==0: eye[e*14+j]=put_tile(b,eye[e*14+j],destripe(sc.tile(b,eye[e*14+j]),0,y*8),y)
ring=A['sc_ring_tab']
for e in range(len(ring)//8):
    for j in range(8):
        x,y=A['sc_ring_x'][j],A['sc_ring_y'][j]; i=y*20+x
        p=attr[i]&7;b=(attr[i]>>3)&1
        ring[e*8+j]=put_tile(b,ring[e*8+j],destripe(sc.tile(b,ring[e*8+j]),p,y*8),y)
# ---- 2. tagline rows 15-17: keep the original x1 pixels/layout (two lines), just type them out
#         earlier (24 half-frames sooner than the x1 export) so both lines sit at full brightness ~0.9 s before the fade (f307)
pool=free[0]
for i in range(300,360):
    if kind[i] and vis[i]: vis[i]-=24
# ---- 4. QC stamp rows 0-1 cols 0-4 (palette 2: BG TL LN RD)
# stamp: red box + REJECTED
for y in range(1,15):
    for x in range(1,39):
        if y in (1,2,13,14) or x in (1,2,37,38): L[y][x]=C['RD']
        elif L[y][x]!=C['LN']: L[y][x]=C['BG']
text("REJECTED",5,5,lambda i:C['RD'])
# remove '+' crosshair conflicts? (top-right untouched)
pal2=[pl[8+j] for j in range(4)]
newvis={}
def repaint(cx,cy,v):
    i=cy*20+cx
    px=[[L[cy*8+r][cx*8+c] for c in range(8)] for r in range(8)]
    if all(px[r][c]==(C['LN'] if stripe_ln(cy*8+r) else C['BG']) for r in range(8) for c in range(8)):
        kind[i]=0; ptile[i]=0; attr[i]=2; vis[i]=0; return
    idx=[[pal2.index(v_) for v_ in row] for row in px]
    kind[i]=1; attr[i]=2|0x08; vis[i]=v      # stamp tiles live in VRAM bank 1 (bank 0 is full)
    ptile[i]=put_tile(1,None,idx,cy)
for cy in (0,1):
    for cx in range(5): repaint(cx,cy,124)
# vis list rebuild
vl=sorted([(vis[i],i%20,i//20) for i in range(360) if kind[i] and vis[i]] )
# keep entries from the original list for cells not touched (same as derived from pos_vis)
A['sc_vis_f2']=[v for v,_,_ in vl];A['sc_vis_x']=[x for _,x,_ in vl];A['sc_vis_y']=[y for _,_,y in vl]
# ---- 3. thin claw slashes over the eye: drop the dark-red edge pixels inside the iris box
ys=[y for y in range(8,56) for x in range(40,120) if L[y][x]==C['TL']]
xs=[x for y in range(8,56) for x in range(40,120) if L[y][x]==C['TL']]
bx0,bx1,by0,by1=min(xs)-2,max(xs)+2,min(ys)-2,max(ys)+2
print('iris box',bx0,bx1,by0,by1)
st=A['sc_slash_tab'][12*18:13*18]
for k,t in enumerate(st):
    if t==255: continue
    sx,sy=A['sc_cell_x'][k],A['sc_cell_y'][k]
    for half in (0,1):
        tt=T[0][t+half]
        for r in range(8):
            for c in range(8):
                X,Y=sx+c,sy+half*8+r
                if bx0<=X<=bx1 and by0<=Y<=by1 and tt[r][c]==2: tt[r][c]=0
# ---- write back
def enc(px):
    out=[]
    for r in range(8):
        lo=hi=0
        for c in range(8):
            lo|=(px[r][c]&1)<<(7-c); hi|=((px[r][c]>>1)&1)<<(7-c)
        out+=[lo,hi]
    return out
n0=max(claimed[0]|set(range(len(G['sc_tiles0'])//16)))+1
n1=max(claimed[1]|set(range(len(G['sc_tiles1'])//16)))+1
t0=sum((enc(T[0][t]) for t in range(n0)),[]); t1=sum((enc(T[1][t]) for t in range(n1)),[])
print(sorted(claimed[1])[-5:],free[1][:3]);print('tiles',n0,n1,'free left',len(free[0]),len(free[1]),'pool left',len(pool))
def fmt(v): return '\n'+'\n'.join('  '+','.join('0x%02x'%x for x in v[i:i+24])+',' for i in range(0,len(v),24))+'\n'
def setarr(s,name,v,dec=False):
    body=('\n  '+','.join(str(x) for x in v)+'\n') if dec else fmt(v)
    s2,n=re.subn(r'((?:static )?const u?int\d+_t %s\[[^\]]*\]=\{)(.*?)(\};)'%name,lambda m:m.group(1)+body+m.group(3),s,flags=re.S)
    assert n==1,name; return s2
d=open('orig/studio_card_data.h').read()
for nm in ('sc_pos_tile','sc_pos_attr','sc_pos_vis','sc_pos_kind','sc_eye_tab','sc_ring_tab','sc_vis_f2','sc_vis_x','sc_vis_y'):
    d=setarr(d,nm,A[nm])
d=setarr(d,'sc_pal_logic',pl,True)
d=re.sub(r'#define SC_VIS_N \d+','#define SC_VIS_N %d'%len(vl),d)
d=re.sub(r'#define SC_TILES0 \d+','#define SC_TILES0 %d'%n0,d); d=re.sub(r'#define SC_TILES1 \d+','#define SC_TILES1 %d'%n1,d)
d=d.replace('// AUTO-GENERATED','// AUTO-GENERATED (then patched by tools/studio_card_patch/mod_card.py: badge stripes, earlier tagline, QC stamp, slashes off the iris)',1)
open(D+'studio_card_data.h','w').write(d)
g=open('orig/studio_card_gfx.h').read(); g=setarr(g,'sc_tiles0',t0); g=setarr(g,'sc_tiles1',t1); open(D+'studio_card_gfx.h','w').write(g)
c=open('orig/studio_card_counts.h').read(); c=re.sub(r'SC_TILES0 \d+','SC_TILES0 %d'%n0,c); c=re.sub(r'SC_TILES1 \d+','SC_TILES1 %d'%n1,c); open(D+'studio_card_counts.h','w').write(c)
