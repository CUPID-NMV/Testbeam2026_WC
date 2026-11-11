#!/bin/bash
#SBATCH --job-name=wcsim
#SBATCH --partition=day
#SBATCH --time=12:00:00
#SBATCH --cpus-per-task=1
#SBATCH --mem=2G
#SBATCH --output=%x_%A_%a.slurm.log
# =======================================================
# bouchet_produci.sh — produzione Hall A (single_pan3) su Bouchet, con
# l'immagine Apptainer della simulazione. Stessa logica di
# produzione_hallA/produci.sh: pezzi da PER_PEZZO eventi, semi per pezzo,
# un .ok per ogni pezzo finito; rilanciando si rifanno solo i pezzi
# mancanti. Qui ogni pezzo e' un elemento di un job array di SLURM.
#
# Uso, sul nodo di login (NON con sbatch: lo script calcola i pezzi e
# sottomette se stesso come array):
#   SIF=~/wcsim/wcsim_28c2c05.sif OUT=~/scratch/wcsim/gamma_24h \
#       bash bouchet_produci.sh <gamma|muoni> [ore] [eventi_per_pezzo]
#
# Variabili:
#   SIF        immagine .sif (obbligatoria)
#   OUT        cartella d'uscita (obbligatoria; meglio su scratch)
#   OTTICA     true: ottica dei pannelli accesa (scintillazione, fibre, SiPM;
#              ~13 s per muone generato). Default false, come produci.sh
#   SEMI       base dei semi (default 10 gamma, 20 muoni: diversa da 1 e 2
#              di produci.sh sul Mac, cosi' le due produzioni sono
#              indipendenti e si possono sommare)
#   MAC_EXTRA  righe di macro prima di /run/initialize, come in produci.sh
#   MACRO_GAMMA macro dei gamma al posto di quella a righe
#   MAXPAR     pezzi in esecuzione insieme (default 200)
#   SBATCH_EXTRA opzioni in piu' per sbatch, es. "--partition=week --time=1-00:00:00"
# =======================================================
set -u

if [ -z "${SLURM_ARRAY_TASK_ID:-}" ]; then
  # ---------- sottomissione, sul nodo di login ----------
  SORGENTE=${1:?sorgente: gamma o muoni}
  ORE=${2:-24}
  PER_PEZZO=${3:-5000000}
  : "${SIF:?manca SIF=<immagine .sif>}" "${OUT:?manca OUT=<cartella d uscita>}"
  [ -f "$SIF" ] || { echo "non trovo $SIF"; exit 1; }
  SIF=$(readlink -f "$SIF")
  mkdir -p "$OUT"; OUT=$(readlink -f "$OUT")
  case "$SORGENTE" in
    gamma) MACRO=${MACRO_GAMMA:-gun_gammas_background_righe_pannelli.mac}; RATE=20608; BASE=${SEMI:-10} ;;
    muoni) MACRO=gun_muons_hallA.mac
           RATE=$(apptainer exec "$SIF" sed -n 's/^# Rate:.*= *\([0-9.eE+-]*\) Hz.*/\1/p' /opt/wcsim/$MACRO)
           BASE=${SEMI:-20} ;;
    *) echo "sorgente sconosciuta: $SORGENTE (gamma o muoni)"; exit 1 ;;
  esac
  [ -n "$RATE" ] || { echo "non riesco a leggere il rate (apptainer exec $SIF funziona?)"; exit 1; }
  NTOT=$(python3 -c "print(round($RATE * $ORE * 3600))")
  NPEZZI=$(( (NTOT + PER_PEZZO - 1) / PER_PEZZO ))
  [ "$NPEZZI" -gt 0 ] || { echo "nessun pezzo da fare ($NTOT eventi)"; exit 1; }
  cat > "$OUT/produzione.txt" <<EOF
sorgente        $SORGENTE
macro           $MACRO
rate_Hz         $RATE
ore             $ORE
eventi_totali   $NTOT
pezzi           $NPEZZI da $PER_PEZZO (l'ultimo il resto)
geometria       single_pan3, onlyActive
ottica_pannelli ${OTTICA:-false}
semi            base $BASE
immagine        $SIF
codice          $(apptainer exec "$SIF" cat /opt/wcsim/GIT_COMMIT)
sottomesso      $(date '+%Y-%m-%d %H:%M')
EOF
  [ -n "${MAC_EXTRA:-}" ] && printf 'macro_extra     %s\n' "$(printf '%s' "$MAC_EXTRA" | tr '\n' ';')" >> "$OUT/produzione.txt"
  echo "$SORGENTE: $NTOT eventi ($ORE h a $RATE Hz) in $NPEZZI pezzi -> $OUT"
  export SIF OUT MACRO NTOT NPEZZI PER_PEZZO BASE MAC_EXTRA="${MAC_EXTRA:-}" OTTICA="${OTTICA:-false}"
  # shellcheck disable=SC2086
  sbatch --array=0-$((NPEZZI - 1))%${MAXPAR:-200} --export=ALL \
         --chdir="$OUT" ${SBATCH_EXTRA:-} "$(readlink -f "$0")"
  exit $?
fi

# ---------- un pezzo, sul nodo di calcolo ----------
k=$SLURM_ARRAY_TASK_ID
nome=$(printf "pezzo_%04d" "$k")
[ -f "$OUT/$nome.ok" ] && { echo "$nome gia' fatto"; exit 0; }
n=$PER_PEZZO
[ "$k" -eq $((NPEZZI - 1)) ] && n=$(( NTOT - PER_PEZZO * (NPEZZI - 1) ))
cat > "$OUT/$nome.mac" <<EOF
/random/setSeeds $(( BASE * 1000000 + k + 1 )) $(( 7919 + 31 * k ))
/control/verbose 0
/run/verbose 0
/wc/panel/optics ${OTTICA:-false}
/wc/output/onlyActive true
EOF
[ -n "$MAC_EXTRA" ] && printf '%s\n' "$MAC_EXTRA" >> "$OUT/$nome.mac"
cat >> "$OUT/$nome.mac" <<EOF
/analysis/setFileName $OUT/$nome
/run/initialize
/control/execute /opt/wcsim/$MACRO
/run/beamOn $n
EOF
t0=$SECONDS
if apptainer exec --cleanenv --bind "$OUT" "$SIF" \
     wcsim single_pan3 "$OUT/$nome.mac" > "$OUT/$nome.log" 2>&1; then
  touch "$OUT/$nome.ok"
  echo "$(date '+%m-%d %H:%M') $nome: $n eventi in $(( SECONDS - t0 )) s"
else
  echo "$(date '+%m-%d %H:%M') $nome: ERRORE, vedi $OUT/$nome.log"
  exit 1
fi
