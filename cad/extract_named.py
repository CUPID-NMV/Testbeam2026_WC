#!/usr/bin/env python3
"""Estrae ogni istanza CON IL NOME della parte dal file STEP (reader CAF),
posizione globale e bounding box. Distingue Tank sensibili da strutturali.
Salva tanks_named.csv."""
import os, glob, csv

from OCC.Core.STEPCAFControl import STEPCAFControl_Reader
from OCC.Core.TDocStd import TDocStd_Document
from OCC.Core.XCAFDoc import XCAFDoc_DocumentTool
from OCC.Core.TCollection import TCollection_ExtendedString
from OCC.Core.TDF import TDF_LabelSequence, TDF_Label
from OCC.Core.TDataStd import TDataStd_Name
from OCC.Core.Bnd import Bnd_Box
from OCC.Core.BRepBndLib import brepbndlib
from OCC.Core.GProp import GProp_GProps
from OCC.Core.BRepGProp import brepgprop

HERE = os.path.dirname(os.path.abspath(__file__))
stp = glob.glob(os.path.join(HERE, "*.stp"))[0]

doc = TDocStd_Document(TCollection_ExtendedString("d"))
reader = STEPCAFControl_Reader()
reader.ReadFile(stp)
reader.Transfer(doc)
st = XCAFDoc_DocumentTool.ShapeTool(doc.Main())

def name_of(label):
    try:
        n = TDataStd_Name()
        if label.FindAttribute(TDataStd_Name.GetID_s(), n):
            return n.Get().ToExtString()
    except Exception:
        pass
    return "?"

rows = []
def walk(label, parent_name=""):
    seq = TDF_LabelSequence()
    st.GetComponents(label, seq)
    if seq.Length() == 0:
        return
    for i in range(1, seq.Length()+1):
        comp = seq.Value(i)
        nm = name_of(comp)
        ref = TDF_Label()
        is_ref = st.GetReferredShape(comp, ref)
        target = ref if is_ref else comp
        tnm = name_of(target) if is_ref else nm
        if st.IsAssembly(target):
            walk(target, tnm)
        else:
            try:
                shape = st.GetShape(comp)   # con la posizione applicata
                if shape is None or shape.IsNull():
                    continue
                box = Bnd_Box()
                brepbndlib.Add(shape, box)
                xmin,ymin,zmin,xmax,ymax,zmax = box.Get()
            except Exception:
                continue
            props = GProp_GProps();
            try: brepgprop.VolumeProperties(shape, props); vol=props.Mass()
            except Exception: vol=0.0
            rows.append(dict(name=str(nm), part=str(tnm),
                xmin=xmin,ymin=ymin,zmin=zmin,xmax=xmax,ymax=ymax,zmax=zmax,
                cx=(xmin+xmax)/2,cy=(ymin+ymax)/2,cz=(zmin+zmax)/2,
                dx=xmax-xmin,dy=ymax-ymin,dz=zmax-zmin,vol_mm3=vol))

tops = TDF_LabelSequence()
st.GetFreeShapes(tops)
print(f"top-level shapes: {tops.Length()}")
for i in range(1, tops.Length()+1):
    walk(tops.Value(i), name_of(tops.Value(i)))

print(f"istanze foglia trovate: {len(rows)}")
from collections import Counter
c = Counter(r["part"] for r in rows)
print("Parti per tipo (top 30):")
for k,v in sorted(c.items(), key=lambda x:-x[1])[:30]:
    print(f"  {v:>3}  {k}")

out = os.path.join(HERE,"tanks_named.csv")
with open(out,"w",newline="") as f:
    w=csv.DictWriter(f,fieldnames=list(rows[0].keys())); w.writeheader(); w.writerows(rows)
print("salvato",out)
