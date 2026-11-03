#!/bin/bash
# =======================================================
# ottica_accesa.sh — eventi con l'ottica dei pannelli ACCESA, divisi su piu'
# processi: per costruire la mappa di luce e per validarla.
#
# Uso, da produzione_hallA/ con l'ambiente di Geant4:
#   bash ottica_accesa.sh <macro_sorgente> <eventi> <etichetta> [processi]
# Esempi:
#   bash ottica_accesa.sh produzione_hallA/mappa_elettroni.mac 7000 mappa
#   bash ottica_accesa.sh gun_gammas_background_righe_pannelli.mac 20000 valida
# (la macro e' relativa alla radice del progetto)
# Uscita: ../data/produzione_hallA/ottica_<etichetta>/parte_NN.root; come
# produci.sh, rilanciando si rifanno solo le parti senza .ok.
# =======================================================
set -u
MACRO_REL=${1:?macro della sorgente}
NTOT=${2:?eventi}
TAG=${3:?etichetta}
NPROC=${4:-10}

QUI=$(cd "$(dirname "$0")" && pwd)
RADICE=$(cd "$QUI/.." && pwd)
EXE="$RADICE/build-arm64/WCtestbeamSim"
MACRO="$RADICE/$MACRO_REL"
[ -f "$MACRO" ] || { echo "manca $MACRO"; exit 1; }
[ -n "${G4LEDATA:-}" ] || { echo "manca l'ambiente di Geant4: conda activate geant4_env"; exit 1; }
OUT="$RADICE/data/produzione_hallA/ottica_$TAG"
mkdir -p "$OUT"
NPER=$(( (NTOT + NPROC - 1) / NPROC ))
echo "$TAG: $NTOT eventi con l'ottica dei pannelli accesa, $NPROC parti da $NPER -> $OUT"

parte() {
  local k=$1 nome; nome=$(printf "parte_%02d" "$k")
  [ -f "$OUT/$nome.ok" ] && return 0
  cat > "$OUT/$nome.mac" <<EOF
/random/setSeeds $(( 3000000 + k + 1 )) $(( 104729 + 17 * k ))
/control/verbose 0
/run/verbose 0
/wc/panel/optics true
/wc/output/onlyActive true
/analysis/setFileName $OUT/$nome
/run/initialize
/control/execute $MACRO
/run/beamOn $NPER
EOF
  local t0=$SECONDS
  if (cd "$OUT" && "$EXE" single_pan3 "$OUT/$nome.mac" > "$OUT/$nome.log" 2>&1); then
    touch "$OUT/$nome.ok"; echo "$(date '+%H:%M') $nome: $NPER eventi in $(( SECONDS - t0 )) s"
  else
    echo "$(date '+%H:%M') $nome: ERRORE, vedi $OUT/$nome.log"
  fi
}
export -f parte
export OUT NPER MACRO EXE
seq 0 $((NPROC - 1)) | caffeinate -i xargs -P "$NPROC" -I{} bash -c 'parte {}'
