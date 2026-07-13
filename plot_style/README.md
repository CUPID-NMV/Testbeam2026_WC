# CUPID camera-ready figure style

A small matplotlib style so our analysis figures (WC test-beam, rate estimate, …)
match the **CUPID publication look**. Palette and conventions were sampled directly
from the CUPID sensitivity paper *Eur. Phys. J. C (2026) 86:633* (the ROOT/EPJC style:
Helvetica, inward ticks on all four sides + minor ticks, no plot title, framed legend,
units in `[...]`, PDF with embedded fonts).

There is no official experiment-provided plotting template, so this reproduces the look
of the published figures.

## Files
- `cupid.mplstyle` — the rcParams stylesheet (fonts, ticks, legend, camera-ready save).
- `cupid_style.py` — semantic colours + helpers (dual T₁/₂ axis, σ bands, IO band, legend, save).
- `example_camera_ready.py` — three worked examples (run it to regenerate the PNG/PDF demos).

## Make it the default (recommended) — no per-script call needed
Install it once as the matplotlib default for your environment:

```bash
python plot_style/install_default.py            # install / re-sync
python plot_style/install_default.py --uninstall  # revert
```

This (a) registers the named style (`plt.style.use("cupid")`), (b) makes
`import cupid_style` work from any working directory, and (c) writes the style
into `~/.matplotlib/matplotlibrc` so **every** script — present and future — gets
the camera-ready look automatically, without calling anything. Re-run it after
editing `cupid.mplstyle`. Scope: the matplotlib config dir is per-user (applies to
all your matplotlib usage); any pre-existing `matplotlibrc` is backed up.

A script that sets its own `rcParams` still wins for the keys it sets (e.g.
`WC_analisys_tool.ipynb` keeps its own font sizes; it still inherits the CUPID
ticks/legend/fonts/palette). For the semantic colours and helpers you still
`import cupid_style` (see below).

## Explicit usage (if you didn't install the default)
At the top of a script or notebook:

```python
import sys; sys.path.insert(0, "plot_style")   # path to this folder
import cupid_style as cs
cs.apply()                                      # <-- camera-ready look for ALL plots
```

or, without the module:

```python
import matplotlib.pyplot as plt
plt.style.use("plot_style/cupid.mplstyle")
```

Then plot as usual — everything already looks right. Use the palette/helpers for the
CUPID-specific elements:

```python
fig, ax = plt.subplots()
cs.io_band(ax, 18.4, 50.0)                       # inverted-ordering region (lavender)
for name in ("EDF", "IBM-2", "pn-QRPA", "Shell"):
    ax.plot(m, prob[name], color=cs.MODELS[name], label=name)
ax.set_xlabel(cs.unit(r"$m_{\beta\beta}$", "meV"))
ax.set_ylabel(cs.unit("Probability of discovery", "%"))
cs.legend(ax)
cs.save(fig, "figure.pdf")                        # writes figure.pdf + figure.png
```

## Palette (semantic — always use these for these curves)
| element | name | hex |
|---|---|---|
| NME model | `cs.MODELS["EDF"]` | `#5DD358` (green) |
| NME model | `cs.MODELS["IBM-2"]` | `#2323FD` (blue) |
| NME model | `cs.MODELS["pn-QRPA"]` | `#FED122` (gold) |
| NME model | `cs.MODELS["Shell"]` | `#F01F1E` (red) |
| 1σ band | `cs.BAND_1SIGMA` | `#4CEA16` |
| 2σ band | `cs.BAND_2SIGMA` | `#FFB74C` |
| inverted ordering | `cs.IO_REGION` | `#CCCBF3` |
| median / central | `cs.MEDIAN` | black |
| reference line | `cs.THRESHOLD` | `#E31A1C` (dashed) |

For generic multi-series plots the default colour cycle (`cs.CYCLE`) is a clean,
distinct publication set — you don't need to set colours by hand.

## Helpers (`cupid_style.py`)
- `cs.apply()` — activate the style.
- `cs.unit(label, "keV")` → `"label [keV]"`.
- `cs.legend(ax)` — framed square-box legend.
- `cs.io_band(ax, lo, hi)` — shaded inverted-ordering region.
- `cs.sigma_bands(ax, x, med, lo1, hi1, lo2, hi2)` — median + 1σ/2σ bands.
- `cs.halflife_top_axis(ax)` — top T₁/₂ axis when the bottom is (T₁/₂)⁻¹.
- `cs.threshold_line(ax, y=…)` or `x=…` — dashed reference line.
- `cs.save(fig, "f.pdf")` — camera-ready PDF (fonts embedded, type-42) + matching PNG.

## Notes
- Fonts: **Helvetica** (falls back to Arial / DejaVu Sans); math in `stixsans` for full Greek (β, σ, …).
- PDFs use `fonttype 42` (no Type-3), as required by most journals.
- Keep plot **titles empty** — put the description in the figure caption, EPJC-style.
