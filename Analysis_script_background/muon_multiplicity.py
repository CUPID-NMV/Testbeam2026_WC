#!/usr/bin/env python3
"""
Studio della MOLTEPLICITA' di moduli accesi sulla geometria REALE del
muon veto di CUPID (da cad/tanks.csv), per verificare se recupera i
muoni radenti che il taglio di carica A+B perde su un singolo modulo.

Fase 1 (questo file): ray-tracing dei muoni cosmici attraverso i 88 moduli,
distribuzione della molteplicita' e dei cammini in acqua per modulo.
"""
import os, csv, json
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CAD  = os.path.join(os.path.dirname(HERE), "cad")
CSV  = os.path.join(CAD, "water_sectors_real.csv")

# --- settori d'acqua dalla GEOMETRIA REALE (CAD 20260713-Assy-WC-Y):
# cavita' interna dei 39 contenitori (bbox meno 10 mm di parete), segmentata in <=40 cm.
# I FORI (cerchio fondo, tetto, porte) sono IMPLICITI: dove non c'e' contenitore non c'e'
# settore -> il buco e' automatico, niente sottrazione analitica.
rows = list(csv.DictReader(open(CSV)))
lo = np.array([[float(r["xmin"]), float(r["ymin"]), float(r["zmin"])] for r in rows])
hi = np.array([[float(r["xmax"]), float(r["ymax"]), float(r["zmax"])] for r in rows])
face = np.array([r["face"] for r in rows])
NMOD = len(rows)
Xr = (lo[:,0].min(), hi[:,0].max()); Yr = (lo[:,1].min(), hi[:,1].max()); Zr = (lo[:,2].min(), hi[:,2].max())
cen = 0.5*(lo+hi); dim = hi-lo
GEOMETRIC_HOLES = True                       # fori impliciti nella disposizione dei contenitori
is_bottom = face == "Zm"; is_top = face == "Zp"
print(f"Settori (geom. reale): {NMOD}  box X{Xr} Y{Yr} Z{Zr} mm  (fori geometrici)")

# --- distribuzione zenitale MACRO (da gun_muons_lngs.mac) ---
_th = np.array([0,.0873,.1745,.2618,.3491,.4363,.5236,.6109,.6981,.7854,.8727,.9599,1.0472,1.1345])
_w  = np.array([3500,3300,3000,2800,2500,2200,1800,1500,1300,1100,900,700,500,150.])
_tg = np.linspace(0, _th[-1], 2000); _wg = np.interp(_tg,_th,_w); _wg /= _wg.sum()

def gen_muons(n, rng):
    """genera n muoni: direzione (MACRO zenith, phi uniforme) + punto su piano sopra il box."""
    th = rng.choice(_tg, size=n, p=_wg)
    ph = rng.uniform(0, 2*np.pi, n)
    d = np.stack([np.sin(th)*np.cos(ph), np.sin(th)*np.sin(ph), -np.cos(th)], axis=1)
    # piano di generazione sopra il box, con margine per tracce inclinate
    H = 1.05*(Zr[1]-Zr[0])*np.tan(_th[-1])   # estensione laterale massima
    z0 = Zr[1] + 10.0
    x0 = rng.uniform(Xr[0]-H, Xr[1]+H, n)
    y0 = rng.uniform(Yr[0]-H, Yr[1]+H, n)
    o = np.stack([x0, y0, np.full(n, z0)], axis=1)
    return o, d

def paths_chunk(o, d):
    """cammino (mm) in ogni modulo per un blocco di raggi. o,d:(M,3) -> L:(M,NMOD)."""
    with np.errstate(divide="ignore", invalid="ignore"):
        inv = 1.0/d                                   # (M,3)
        t1 = (lo[None,:,:] - o[:,None,:]) * inv[:,None,:]   # (M,NMOD,3)
        t2 = (hi[None,:,:] - o[:,None,:]) * inv[:,None,:]
        tmin = np.maximum(np.minimum(t1,t2), None).max(axis=2) if False else np.minimum(t1,t2).max(axis=2)
        tmax = np.maximum(t1,t2).min(axis=2)
    L = np.where((tmax > tmin) & (tmax > 0), tmax - np.maximum(tmin,0.0), 0.0)   # |d|=1
    return L

def _carve_circle(o, d, L, mask, circ):
    """azzera il cammino nei settori di 'mask' dove il muone passa nel cerchio (foro)."""
    zc = cen[mask, 2]
    t = (zc[None,:] - o[:,2:3]) / d[:,2:3]              # (m, n_mask) al piano z del settore
    x = o[:,0:1] + t*d[:,0:1]; y = o[:,1:2] + t*d[:,1:2]
    inside = ((x-circ["c"][0])**2 + (y-circ["c"][1])**2) < circ["r"]**2
    Lm = L[:, mask]; Lm[inside] = 0.0; L[:, mask] = Lm

def _carve_port(o, d, L, p):
    """azzera il cammino sui settori di una parete dove il muone passa nella porta rett."""
    fmask = face == p["face"]
    nrm, wax = p["nrm"], p["wax"]
    with np.errstate(divide="ignore", invalid="ignore"):
        t = (p["ncoord"] - o[:,nrm:nrm+1]) / d[:,nrm:nrm+1]   # al piano della parete
        w = o[:,wax:wax+1] + t*d[:,wax:wax+1]; z = o[:,2:3] + t*d[:,2:3]
    inside = (w > p["wlo"]) & (w < p["whi"]) & (z > p["zlo"]) & (z < p["zhi"])
    Lw = L[:, fmask]; Lw[inside[:,0], :] = 0.0; L[:, fmask] = Lw

def apply_hole(o, d, L, use_hole=True):
    """fori impliciti nella geometria reale dei contenitori -> nessuna sottrazione analitica."""
    return L

# --- campionatore risposta in luce: dato L (mm) -> (a,b) in PE dal MC ---
class LightModel:
    def __init__(self, tag):
        z = np.load(os.path.join(HERE, f"mc_response_{tag}.npz"))
        o = np.argsort(z["L"])
        self.Ls = z["L"][o]; self.As = z["a"][o]; self.Bs = z["b"][o]
        self.n = len(self.Ls); self.tag = tag
    def sample(self, Lq, rng, delta=15.0):
        ilo = np.searchsorted(self.Ls, Lq - delta)
        ihi = np.searchsorted(self.Ls, Lq + delta)
        span = np.maximum(ihi - ilo, 1)
        pick = np.clip(ilo + (rng.random(len(Lq)) * span).astype(int), 0, self.n - 1)
        near = np.clip(np.searchsorted(self.Ls, Lq), 0, self.n - 1)
        pick = np.where(ihi <= ilo, near, pick)
        return self.As[pick], self.Bs[pick]

def run(tag, N=300_000, SUM_CUT=5.2, LIGHT_THR=1.0, seed=1, use_hole=True):
    rng = np.random.default_rng(seed)
    lm = LightModel(tag)
    o, d = gen_muons(N, rng)
    # accumulatori per muone (solo quelli che colpiscono >=1 modulo)
    n_mod=[]; n_lit=[]; n_pass=[]; Lmax_l=[]
    CH = 20_000
    for s in range(0, N, CH):
        L = paths_chunk(o[s:s+CH], d[s:s+CH])          # (m,NMOD)
        L = apply_hole(o[s:s+CH], d[s:s+CH], L, use_hole)
        cross = L > 0.5
        m = cross.sum(axis=1)
        hitmask = m > 0
        # flatten moduli attraversati
        mi, mj = np.where(cross)                         # indici (muone, modulo) nel chunk
        Lq = L[mi, mj]
        a, b = lm.sample(Lq, rng)
        lit  = (a + b) > LIGHT_THR                       # modulo con luce
        pas  = (a + b) > SUM_CUT                          # modulo che supera il veto A+B
        # aggrega per muone
        nlit = np.bincount(mi, weights=lit, minlength=L.shape[0])
        npas = np.bincount(mi, weights=pas, minlength=L.shape[0])
        n_mod.append(m[hitmask]); n_lit.append(nlit[hitmask].astype(int))
        n_pass.append(npas[hitmask].astype(int))
        Lmax_l.append(np.where(cross, L, 0).max(axis=1)[hitmask])
    n_mod=np.concatenate(n_mod); n_lit=np.concatenate(n_lit)
    n_pass=np.concatenate(n_pass); Lmax=np.concatenate(Lmax_l)
    Nhit=len(n_mod)

    # logiche di veto
    tag_AB   = n_pass >= 1                 # (1) singolo modulo: qualche modulo supera A+B
    tag_mult = n_lit  >= 2                 # (2) molteplicita': >=2 moduli con luce
    tag_comb = tag_AB | tag_mult           # (3) OR

    print(f"\n===== {tag}  (SUM_CUT={SUM_CUT} PE, luce>{LIGHT_THR} PE, N_hit={Nhit}) =====")
    print(f"Molteplicita' moduli attraversati: mediana {int(np.median(n_mod))}, >=2: {100*(n_mod>=2).mean():.1f}%")
    print(f"Efficienza veto muone (denom = muoni che colpiscono il box):")
    print(f"  (1) A+B singolo modulo (OR):        {100*tag_AB.mean():.2f}%")
    print(f"  (2) molteplicita' >=2 moduli:       {100*tag_mult.mean():.2f}%")
    print(f"  (3) combinata (1 OR 2):             {100*tag_comb.mean():.2f}%")
    miss1 = ~tag_AB
    rec = miss1 & tag_mult
    print(f"\nMuoni PERSI dal solo A+B: {miss1.sum()} ({100*miss1.mean():.2f}%)")
    print(f"  di cui recuperati dalla molteplicita' >=2: {rec.sum()} ({100*rec.sum()/max(miss1.sum(),1):.1f}%)")
    print(f"Muoni persi ANCHE dalla combinata: {(~tag_comb).sum()} ({100*(~tag_comb).mean():.3f}%)")
    return dict(n_mod=n_mod,n_lit=n_lit,n_pass=n_pass,Lmax=Lmax,
                tag_AB=tag_AB,tag_mult=tag_mult,tag_comb=tag_comb)

if __name__ == "__main__":
    for tag in ["single_wbc2","single_wbc3"]:
        run(tag)
