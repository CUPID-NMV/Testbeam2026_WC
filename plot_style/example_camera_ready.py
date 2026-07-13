"""
Demo of the CUPID camera-ready style. Run:  python example_camera_ready.py
Produces three example figures (pdf + png) in this folder.
"""
import os, sys
import numpy as np
import matplotlib.pyplot as plt

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cupid_style as cs

cs.apply()
OUT = os.path.dirname(os.path.abspath(__file__))


def sigmoid(x, x0, k):
    return 100.0 / (1.0 + np.exp(-(x - x0) / k))


# 1) Discovery probability vs m_bb — the CUPID NME-model palette + IO band (Fig. 6 right)
def fig_discovery():
    m = np.linspace(0, 60, 400)
    fig, ax = plt.subplots()
    cs.io_band(ax, 18.4, 50.0)                     # inverted-ordering region (lavender)
    for name, x0, k in [("EDF", 11, 2.0), ("IBM-2", 15, 2.3),
                        ("pn-QRPA", 21, 2.6), ("Shell", 34, 4.5)]:
        ax.plot(m, sigmoid(m, x0, k), color=cs.MODELS[name], label=name)
    ax.set_xlim(0, 60); ax.set_ylim(0, 105)
    ax.set_xlabel(cs.unit(r"$m_{\beta\beta}$", "meV"))
    ax.set_ylabel(cs.unit("Probability of discovery", "%"))
    cs.legend(ax, loc="center right")
    cs.save(fig, os.path.join(OUT, "example_discovery_prob.pdf"))
    plt.close(fig)


# 2) p-value with median + 1σ/2σ bands and the T_1/2 dual axis (Fig. 5)
def fig_pvalue():
    x = np.linspace(0.001, 1.25, 300)             # (T_1/2)^-1 [1e27 yr^-1]
    med = 10 ** (-0.1 - 3.0 * x)
    lo1, hi1 = med * 0.35, med * 2.8
    lo2, hi2 = med * 0.12, med * 8.0
    fig, ax = plt.subplots()
    cs.sigma_bands(ax, x, med, lo1, hi1, lo2, hi2, label_med="median")
    cs.threshold_line(ax, y=1.4e-3, label=r"$3\sigma$")
    ax.set_yscale("log")
    ax.set_xlim(0.02, 1.25); ax.set_ylim(1e-4, 1)
    ax.set_xlabel(cs.unit(r"$(T_{1/2})^{-1}$", r"$10^{27}\,\mathrm{yr^{-1}}$"))
    ax.set_ylabel("p-value")
    sec = cs.halflife_top_axis(ax)
    sec.set_xticks([1, 2, 5, 10])
    cs.legend(ax, loc="lower left")
    cs.save(fig, os.path.join(OUT, "example_pvalue_bands.pdf"))
    plt.close(fig)


# 3) A generic analysis plot (efficiency vs position) in the same style
def fig_efficiency():
    x = np.linspace(-48, 48, 17)
    eff = 99 - 0.0016 * x**2 + np.random.default_rng(1).normal(0, 0.4, x.size)
    err = np.full_like(x, 0.6)
    fig, ax = plt.subplots()
    ax.errorbar(x, eff, yerr=err, fmt="o", color=cs.CYCLE[0],
                mfc=cs.CYCLE[0], mec="black", label="DATA")
    cs.threshold_line(ax, y=99, label="99% target")
    ax.set_xlim(-52, 52); ax.set_ylim(80, 102)
    ax.set_xlabel(cs.unit("X", "cm"))
    ax.set_ylabel(cs.unit("Efficiency", "%"))
    cs.legend(ax, loc="lower center")
    cs.save(fig, os.path.join(OUT, "example_efficiency.pdf"))
    plt.close(fig)


if __name__ == "__main__":
    fig_discovery()
    fig_pvalue()
    fig_efficiency()
    print("wrote example_discovery_prob / example_pvalue_bands / example_efficiency (.pdf + .png)")
