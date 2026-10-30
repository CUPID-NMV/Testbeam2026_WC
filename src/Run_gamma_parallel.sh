#!/bin/bash
# =======================================================
# Run_gamma_parallel.sh — gamma di Hall A divisi su N processi
#
# Uso (da src/, con conda activate geant4_env):
#   bash Run_gamma_parallel.sh <geo> <macro_gps> <tag> <eventi_totali> [n_processi] [fotoni_per_MeV]
# Esempio:
#   bash Run_gamma_parallel.sh single gun_gammas_background_righe.mac gamma_righe 10000000 8
#
# Perche' piu' processi: il programma gira in modo SEQUENZIALE (il log dice
# "sequential application", /run/numberOfThreads viene ignorato), quindi un
# solo processo usa un solo core. Ogni processo ha i suoi semi casuali: con
# quelli di default di Geant4 genererebbero tutti gli stessi eventi.
#
# Uscita: ../data/sim_<geo>_<tag>_part<k>_X0mm_Y0mm_Th0deg.root, k = 1..N.
# Eseguibile: ../build-arm64/WCtestbeamSim (Geant4 11.4.2, Apple Silicon).
# =======================================================

GEO=${1:?geometria}
GPS_MAC=${2:?macro GPS}
TAG=${3:?etichetta}
NTOT=${4:?eventi totali}
NJOBS=${5:-8}
# resa della scintillazione del nucleo delle fibre (FiberScintillation.hh);
# vuoto = quella di serie del programma (1750 fotoni/MeV), 0 = spenta
SCINT=${6:-}
EXECUTABLE="../build-arm64/WCtestbeamSim"
NPER=$(( (NTOT + NJOBS - 1) / NJOBS ))

mkdir -p ../data tmp_jobs
echo "geo=$GEO  macro=$GPS_MAC  tag=$TAG  eventi=$NTOT in $NJOBS processi da $NPER  scintillazione fibre: ${SCINT:-di serie}"
for k in $(seq 1 $NJOBS); do
    OUT="../data/sim_${GEO}_${TAG}_part${k}_X0mm_Y0mm_Th0deg.root"
    MAC="tmp_jobs/run_${GEO}_${TAG}_part${k}.mac"
    LOG="tmp_jobs/log_${GEO}_${TAG}_part${k}.txt"
    cat <<EOF > "$MAC"
/random/setSeeds $((12345 + 1000 * k)) $((67890 + 7 * k))
${SCINT:+/wc/fiber/scintYield $SCINT}
/analysis/setFileName $OUT
/run/initialize
/control/execute ../$GPS_MAC
/run/beamOn $NPER
EOF
    echo " -> $OUT"
    nohup $EXECUTABLE $GEO "$MAC" > "$LOG" 2>&1 &
done
wait
echo "FATTO: $(ls ../data/sim_${GEO}_${TAG}_part*_X0mm_Y0mm_Th0deg.root | wc -l) file"
grep -l "Fatal Exception" tmp_jobs/log_${GEO}_${TAG}_part*.txt && echo "ATTENZIONE: qualche processo si e' fermato con un'eccezione fatale"
