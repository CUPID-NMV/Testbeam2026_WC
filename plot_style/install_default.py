#!/usr/bin/env python3
"""
Make the CUPID camera-ready style the matplotlib DEFAULT for this user.

    python plot_style/install_default.py            # install / re-sync
    python plot_style/install_default.py --uninstall

What it does (all reversible):
  1. copies cupid.mplstyle into matplotlib's stylelib  -> plt.style.use("cupid")
  2. drops a .pth into site-packages                   -> `import cupid_style` works anywhere
  3. writes the style into <configdir>/matplotlibrc    -> default look for EVERY script,
     present and future, without calling anything (existing matplotlibrc is backed up).

Re-run after editing cupid.mplstyle to re-sync the default.
"""
import matplotlib, os, sys, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
SRC  = os.path.join(HERE, "cupid.mplstyle")
CFG  = matplotlib.get_configdir()
STYLELIB = os.path.join(CFG, "stylelib")
RC   = os.path.join(CFG, "matplotlibrc")
BACKUP = RC + ".pre-cupid-backup"
PTH_NAME = "cupid_style_path.pth"
MARK = "# CUPID camera-ready default style — installed by plot_style/install_default.py"


def _site_packages():
    for p in sys.path:
        if p.endswith("site-packages") and os.path.isdir(p):
            return p
    import site
    return (site.getsitepackages() or [site.getusersitepackages()])[0]


def install():
    os.makedirs(STYLELIB, exist_ok=True)
    # 1) named style
    shutil.copy(SRC, os.path.join(STYLELIB, "cupid.mplstyle"))
    # 2) make the module importable from any working directory
    sp = _site_packages()
    with open(os.path.join(sp, PTH_NAME), "w") as f:
        f.write(HERE + "\n")
    # 3) default matplotlibrc  (back up a pre-existing, non-ours rc once)
    if os.path.exists(RC):
        first = open(RC).readline()
        if MARK not in first and not os.path.exists(BACKUP):
            shutil.copy(RC, BACKUP)
            print(f"  backed up existing matplotlibrc -> {BACKUP}")
    with open(RC, "w") as f:
        f.write(MARK + "\n")
        f.write("# To remove: python plot_style/install_default.py --uninstall\n\n")
        f.write(open(SRC).read())
    print("Installed CUPID style as matplotlib default:")
    print("  style   :", os.path.join(STYLELIB, "cupid.mplstyle"))
    print("  import  :", os.path.join(sp, PTH_NAME), "->", HERE)
    print("  default :", RC)


def uninstall():
    removed = []
    p1 = os.path.join(STYLELIB, "cupid.mplstyle")
    if os.path.exists(p1):
        os.remove(p1); removed.append(p1)
    sp = _site_packages()
    pth = os.path.join(sp, PTH_NAME)
    if os.path.exists(pth):
        os.remove(pth); removed.append(pth)
    if os.path.exists(RC) and MARK in open(RC).readline():
        if os.path.exists(BACKUP):
            shutil.move(BACKUP, RC); removed.append(RC + " (restored backup)")
        else:
            os.remove(RC); removed.append(RC)
    print("Uninstalled. Removed/restored:")
    for r in removed:
        print("  -", r)


if __name__ == "__main__":
    (uninstall if "--uninstall" in sys.argv else install)()
