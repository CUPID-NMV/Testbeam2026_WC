#!/usr/bin/env python3
"""Estrae posizione, orientazione e bounding box (globali) di ogni tank
dal file STEP dell'assieme WC muon veto di CUPID. Salva in tanks.csv."""
import sys, csv, glob, os

from OCC.Core.STEPControl import STEPControl_Reader
from OCC.Core.IFSelect import IFSelect_RetDone
from OCC.Core.TopExp import TopExp_Explorer
from OCC.Core.TopAbs import TopAbs_SOLID
from OCC.Core.TopoDS import topods
from OCC.Core.Bnd import Bnd_Box
from OCC.Core.BRepBndLib import brepbndlib
from OCC.Core.GProp import GProp_GProps
from OCC.Core.BRepGProp import brepgprop

HERE = os.path.dirname(os.path.abspath(__file__))
stp = glob.glob(os.path.join(HERE, "*.stp"))[0]
print("File:", os.path.basename(stp))

reader = STEPControl_Reader()
status = reader.ReadFile(stp)
if status != IFSelect_RetDone:
    print("ERRORE lettura STEP"); sys.exit(1)
reader.TransferRoots()
shape = reader.OneShape()
print("Trasferito. Esploro i solidi...")

rows = []
exp = TopExp_Explorer(shape, TopAbs_SOLID)
i = 0
while exp.More():
    solid = topods.Solid(exp.Current())
    # bounding box globale
    box = Bnd_Box()
    brepbndlib.Add(solid, box)
    xmin, ymin, zmin, xmax, ymax, zmax = box.Get()
    # volume + centro di massa
    props = GProp_GProps()
    brepgprop.VolumeProperties(solid, props)
    vol = props.Mass()
    com = props.CentreOfMass()
    rows.append(dict(
        idx=i,
        cx=com.X(), cy=com.Y(), cz=com.Z(),
        xmin=xmin, ymin=ymin, zmin=zmin, xmax=xmax, ymax=ymax, zmax=zmax,
        dx=xmax-xmin, dy=ymax-ymin, dz=zmax-zmin,
        vol_mm3=vol))
    i += 1
    exp.Next()

print(f"Solidi trovati: {len(rows)}")
# estensione globale dell'assieme
if rows:
    ax = [min(r["xmin"] for r in rows), max(r["xmax"] for r in rows)]
    ay = [min(r["ymin"] for r in rows), max(r["ymax"] for r in rows)]
    az = [min(r["zmin"] for r in rows), max(r["zmax"] for r in rows)]
    print(f"Estensione assieme (mm): "
          f"X[{ax[0]:.0f},{ax[1]:.0f}]={ax[1]-ax[0]:.0f}  "
          f"Y[{ay[0]:.0f},{ay[1]:.0f}]={ay[1]-ay[0]:.0f}  "
          f"Z[{az[0]:.0f},{az[1]:.0f}]={az[1]-az[0]:.0f}")

out = os.path.join(HERE, "tanks.csv")
with open(out, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
    w.writeheader(); w.writerows(rows)
print("salvato:", out)

# stampa i primi solidi per dimensione (i tank grandi = moduli)
rows_sorted = sorted(rows, key=lambda r: -r["vol_mm3"])
print("\nTop 10 solidi per volume (mm), centro e dimensioni:")
print(f"{'idx':>3} {'vol(L)':>8} | {'cx':>8} {'cy':>8} {'cz':>8} | {'dx':>7} {'dy':>7} {'dz':>7}")
for r in rows_sorted[:10]:
    print(f"{r['idx']:>3} {r['vol_mm3']/1e6:>8.2f} | {r['cx']:>8.0f} {r['cy']:>8.0f} {r['cz']:>8.0f} | "
          f"{r['dx']:>7.0f} {r['dy']:>7.0f} {r['dz']:>7.0f}")
