#!/bin/bash
# =======================================================
# produci.sh — produzione Hall A (WC + pannelli, single_pan3) a pezzi
#
# Uso, da produzione_hallA/ con l'ambiente di Geant4 (conda activate geant4_env):
#   bash produci.sh <sorgente> [ore] [processi] [eventi_per_pezzo]
#     sorgente  gamma: gun_gammas_background_righe_pannelli.mac, 20608 Hz
#               muoni: gun_muons_hallA.mac, il rate scritto nella macro
#     ore       tempo equivalente di presa dati (default 24)
#     processi  in parallelo (default 10, i core del Mac)
#     eventi_per_pezzo  default 5000000 (~1.5 ore di un processo, gamma)
#
# Ottica dei pannelli spenta (la luce si mette dopo con la mappa), ottica del
# WC accesa, /wc/output/onlyActive true. Ogni pezzo ha i suoi semi.
# Uscita: ../data/produzione_hallA/<sorgente>_<ore>h/pezzo_NNNN.root, il log
# e un .ok quando il pezzo e' finito. Rilanciando lo stesso comando si rifanno
# solo i pezzi senza .ok: una produzione interrotta riparte da li'.
# Il Mac non va in stop finche' gira (caffeinate), ma col coperchio chiuso si'.
# =======================================================
set -u
SORGENTE=${1:?sorgente: gamma o muoni}
ORE=${2:-24}
NPROC=${3:-10}
PER_PEZZO=${4:-5000000}

QUI=$(cd "$(dirname "$0")" && pwd)
RADICE=$(cd "$QUI/.." && pwd)
EXE="$RADICE/build-arm64/WCtestbeamSim"
case "$SORGENTE" in
  gamma) MACRO="$RADICE/gun_gammas_background_righe_pannelli.mac"; RATE=20608; BASE=1 ;;
  muoni) MACRO="$RADICE/gun_muons_hallA.mac"
         RATE=$(sed -n 's/^# Rate:.*= *\([0-9.eE+-]*\) Hz.*/\1/p' "$MACRO"); BASE=2 ;;
  *) echo "sorgente sconosciuta: $SORGENTE (gamma o muoni)"; exit 1 ;;
esac
[ -x "$EXE" ] || { echo "manca $EXE: compilare in build-arm64"; exit 1; }
[ -n "${G4LEDATA:-}" ] || { echo "manca l'ambiente di Geant4: conda activate geant4_env"; exit 1; }

NTOT=$(python3 -c "print(round($RATE * $ORE * 3600))")
NPEZZI=$(( (NTOT + PER_PEZZO - 1) / PER_PEZZO ))
OUT="$RADICE/data/produzione_hallA/${SORGENTE}_${ORE}h"
mkdir -p "$OUT"
cat > "$OUT/produzione.txt" <<EOF
sorgente        $SORGENTE
macro           $MACRO
rate_Hz         $RATE
ore             $ORE
eventi_totali   $NTOT
pezzi           $NPEZZI da $PER_PEZZO (l'ultimo il resto)
geometria       single_pan3, ottica dei pannelli spenta, onlyActive
codice          $(git -C "$RADICE" rev-parse --short HEAD)$(git -C "$RADICE" diff --quiet -- src include sim.cc || echo ' (modificato)')
inizio          $(date '+%Y-%m-%d %H:%M')
EOF
echo "$SORGENTE: $NTOT eventi ($ORE h a $RATE Hz) in $NPEZZI pezzi, $NPROC processi -> $OUT"

pezzo() {   # $1 = indice del pezzo, da 0
  local k=$1 nome; nome=$(printf "pezzo_%04d" "$k")
  [ -f "$OUT/$nome.ok" ] && return 0
  local n=$PER_PEZZO
  [ "$k" -eq $((NPEZZI - 1)) ] && n=$(( NTOT - PER_PEZZO * (NPEZZI - 1) ))
  cat > "$OUT/$nome.mac" <<EOF
/random/setSeeds $(( BASE * 1000000 + k + 1 )) $(( 7919 + 31 * k ))
/control/verbose 0
/run/verbose 0
/wc/panel/optics false
/wc/output/onlyActive true
/analysis/setFileName $OUT/$nome
/run/initialize
/control/execute $MACRO
/run/beamOn $n
EOF
  local t0=$SECONDS
  if (cd "$OUT" && "$EXE" single_pan3 "$OUT/$nome.mac" > "$OUT/$nome.log" 2>&1); then
    touch "$OUT/$nome.ok"
    echo "$(date '+%m-%d %H:%M') $nome: $n eventi in $(( SECONDS - t0 )) s  ($(ls "$OUT"/*.ok | wc -l | tr -d ' ')/$NPEZZI)"
  else
    echo "$(date '+%m-%d %H:%M') $nome: ERRORE, vedi $OUT/$nome.log"
  fi
}
export -f pezzo
export OUT NPEZZI NTOT PER_PEZZO BASE MACRO EXE

seq 0 $((NPEZZI - 1)) | caffeinate -i xargs -P "$NPROC" -I{} bash -c 'pezzo {}'
FATTI=$(ls "$OUT"/*.ok 2>/dev/null | wc -l | tr -d ' ')
echo "fine          $(date '+%Y-%m-%d %H:%M'), $FATTI/$NPEZZI pezzi" >> "$OUT/produzione.txt"
echo "$FATTI/$NPEZZI pezzi finiti"
