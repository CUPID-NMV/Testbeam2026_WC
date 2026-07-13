"""
CUPID camera-ready plotting style.

Palette and conventions sampled from the CUPID sensitivity paper
Eur. Phys. J. C (2026) 86:633 (arXiv/EPJC), so your figures match the
collaboration publication look.

Quick start
-----------
    import sys; sys.path.insert(0, "plot_style")   # or wherever this file is
    import cupid_style as cs
    cs.apply()                                      # sets the camera-ready rcParams

    import matplotlib.pyplot as plt
    fig, ax = plt.subplots()
    ax.plot(x, y_edf,   color=cs.MODELS["EDF"],    label="EDF")
    ax.plot(x, y_ibm2,  color=cs.MODELS["IBM-2"],  label="IBM-2")
    cs.io_band(ax, 18.4, 50.0)                      # inverted-ordering region
    cs.legend(ax)
    cs.save(fig, "myfigure.pdf")                    # writes .pdf (+ .png)

Everything the style file (`cupid.mplstyle`) does not cover — semantic
colours, the T_1/2 dual axis, sigma bands, framed legend, saving — lives here.
"""
import os
import matplotlib.pyplot as plt

_HERE = os.path.dirname(os.path.abspath(__file__))
STYLE = os.path.join(_HERE, "cupid.mplstyle")

# ------------------------------------------------------------------ palette ---
# CUPID NME models (semantic colours — always use these for these curves)
MODELS = {
    "EDF":     "#5DD358",   # green
    "IBM-2":   "#2323FD",   # blue
    "pn-QRPA": "#FED122",   # gold
    "Shell":   "#F01F1E",   # red
}

BAND_1SIGMA = "#4CEA16"   # 1σ band (green)
BAND_2SIGMA = "#FFB74C"   # 2σ band (orange)
IO_REGION   = "#CCCBF3"   # inverted-ordering shaded region (lavender)
MEDIAN      = "black"      # median / central curve
THRESHOLD   = "#E31A1C"   # dashed reference lines (e.g. 3σ, 90% CL)

# General categorical cycle (same as the style file; exposed for convenience)
CYCLE = ["#1a4fd6", "#e8000b", "#2ca02c", "#f5a800",
         "#7a3fbf", "#17a0a0", "#8c564b", "#444444"]


# -------------------------------------------------------------------- setup ---
def apply():
    """Activate the CUPID camera-ready style for all subsequent plots."""
    plt.style.use(STYLE)

use = apply  # alias


# ----------------------------------------------------------------- helpers ----
def unit(label, u=None):
    """Axis label with the unit in square brackets, EPJC convention."""
    return f"{label} [{u}]" if u else label


def legend(ax=None, loc="best", **kw):
    """Framed square-box legend (ROOT TLegend look)."""
    ax = ax or plt.gca()
    kw.setdefault("frameon", True)
    kw.setdefault("edgecolor", "black")
    kw.setdefault("fancybox", False)
    lg = ax.legend(loc=loc, **kw)
    if lg is not None:
        lg.get_frame().set_linewidth(1.0)
    return lg


def io_band(ax=None, lo=None, hi=None, label="Inverted ordering"):
    """Shade the inverted-ordering m_bb region (lavender), behind everything."""
    ax = ax or plt.gca()
    return ax.axvspan(lo, hi, color=IO_REGION, linewidth=0, zorder=0, label=label)


def sigma_bands(ax, x, med, lo1, hi1, lo2, hi2,
                label_med="median", label_1=r"$\pm1\sigma$", label_2=r"$\pm2\sigma$"):
    """Median black curve with 1σ (green) and 2σ (orange) bands (Fig. 5 look)."""
    ax.fill_between(x, lo2, hi2, color=BAND_2SIGMA, linewidth=0, zorder=1, label=label_2)
    ax.fill_between(x, lo1, hi1, color=BAND_1SIGMA, linewidth=0, zorder=2, label=label_1)
    ax.plot(x, med, color=MEDIAN, linewidth=1.8, zorder=3, label=label_med)


def halflife_top_axis(ax, label=r"$T_{1/2}$  [yr]"):
    """
    Add a top axis showing T_1/2 = 1/x when the bottom axis is (T_1/2)^-1
    (the CUPID sensitivity dual-axis, Figs. 5-7).
    """
    import numpy as np
    def inv(v):
        v = np.asarray(v, dtype=float)
        # clamp near-zero so 1/x stays finite (bottom axis may include 0)
        v = np.where(np.abs(v) < 1e-6, np.sign(v + 1e-30) * 1e-6, v)
        return 1.0 / v
    sec = ax.secondary_xaxis("top", functions=(inv, inv))
    sec.set_xlabel(label)
    return sec


def threshold_line(ax, y=None, x=None, label=None):
    """Dashed red reference line (horizontal if y given, vertical if x given)."""
    if y is not None:
        return ax.axhline(y, color=THRESHOLD, ls="--", lw=1.3, label=label, zorder=4)
    return ax.axvline(x, color=THRESHOLD, ls="--", lw=1.3, label=label, zorder=4)


def save(fig, path, also_png=True):
    """Save camera-ready: PDF (fonts embedded, type-42) and a matching PNG."""
    fig.savefig(path, bbox_inches="tight")
    if also_png and path.lower().endswith(".pdf"):
        fig.savefig(path[:-4] + ".png", dpi=300, bbox_inches="tight")
    return path
