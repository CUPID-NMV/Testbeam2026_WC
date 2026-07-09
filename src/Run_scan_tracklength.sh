#!/bin/bash
# =======================================================
# Run_scan_tracklength.sh — Produzione mirata: lunghezza
# traccia primaria in acqua controllata tramite fascio
# inclinato (NON generato dentro l'acqua).
#
# Uso: bash Run_scan_tracklength.sh [geo]
#   geo: single | triple   (default: single)
#
# Il fascio e- 500 MeV entra normalmente dalla faccia
# frontale dell'acqua (Z=+50mm, come nel testbeam reale) ma
# e' inclinato di un angolo fisso THETA nel piano XZ ed esce
# lateralmente dalla parete X=+480mm PRIMA di raggiungere la
# faccia posteriore (Z=-50mm). La posizione X di impatto e'
# scelta runa per run in modo che la lunghezza del segmento
# percorso in acqua sia esattamente quella richiesta.
#
# Geometria acqua (single, vedi SingleModuleDetector.cc):
#   waterX = 480 mm (semilarghezza), waterZ = 50 mm (semispessore)
#
# Per un fascio con direzione (sinT, 0, -cosT) che entra in
# (xe, +50) ed esce dopo un percorso L sulla parete X=+480:
#   xe       = 480 - L*sin(theta)
#   X_target = 480 - L*sin(theta) + 50*tan(theta)   (posizione
#              nominale a Z=0, stile "logbook"/Run_scan.sh)
#   X_START  = X_target - Z_GEN*tan(theta)          (posizione
#              gun a Z=Z_GEN, da passare a /gps/pos/centre)
#
# Lunghezze richieste: 10, 9, 8, 7, 6, 5, 4, 2, 0.5 cm
# Naming output (compatibile con la convenzione standard,
# cosi' il notebook di analisi puo' leggerli come run normali
# e ricostruire la lunghezza di traccia con la stessa formula):
#   sim_<geo>_e_tb_X<X_target>mm_Y0mm_Th<theta>deg.root
# =======================================================

GEO=${1:-single}
EXECUTABLE="../build/WCtestbeamSim"
EVENTS=1000
Z_GEN=2000.0   # mm — piano di generazione fascio (gun)
THETA=60       # gradi — angolo di inclinazione fascio nel piano XZ

# Lunghezze traccia desiderate in acqua (mm)
LENGTHS_MM=(100 90 80 70 60 50 40 20 5)

case $GEO in
    single|triple) ;;
    *)
        echo "ERRORE: geometria non valida '$GEO'"
        echo "       valori validi: single | triple"
        exit 1
        ;;
esac

mkdir -p ../data
mkdir -p tmp_jobs

DX=$(awk -v t="$THETA" 'BEGIN{printf "%.6f", sin(t*3.14159265358979/180)}')
DZ=$(awk -v t="$THETA" 'BEGIN{printf "%.6f", -cos(t*3.14159265358979/180)}')
TANT=$(awk -v t="$THETA" 'BEGIN{printf "%.6f", sin(t*3.14159265358979/180)/cos(t*3.14159265358979/180)}')

echo "==================================================="
echo " PRODUZIONE WC Testbeam 2026 — scan lunghezza traccia"
echo " Geometria : $GEO"
echo " Sorgente  : e- 500 MeV (gun_electrons_testbeam.mac)"
echo " Angolo    : ${THETA} deg (fascio inclinato, esce dal lato)"
echo " Eventi/run: $EVENTS"
echo " Output    : ../data/"
echo "==================================================="

NJOBS=0
for L in "${LENGTHS_MM[@]}"; do

    # X_TARGET arrotondato all'mm intero: il naming "sim_..._X<int>mm_..."
    # deve restare compatibile con la regex di Signal_maker (X(m?\d+)mm,
    # niente punti decimali). X_START e' poi ricavato dallo stesso valore
    # arrotondato, cosi' nome file e fascio realmente generato coincidono.
    X_TARGET=$(awk -v l="$L" -v t="$TANT" \
        'BEGIN{th=atan2(t,1); printf "%.0f", 480.0 - l*sin(th) + 50.0*t}')
    X_START=$(awk -v xt="$X_TARGET" -v z="$Z_GEN" -v t="$TANT" \
        'BEGIN{printf "%.2f", xt - z*t}')
    X_STR=$(echo "$X_TARGET" | sed 's/-/m/')

    OUTFILE="../data/sim_${GEO}_e_tb_X${X_STR}mm_Y0mm_Th${THETA}deg.root"
    MACRO_FILE="tmp_jobs/run_${GEO}_e_tb_tracklen_L${L}mm.mac"
    LOG_FILE="tmp_jobs/log_${GEO}_e_tb_tracklen_L${L}mm.txt"

    cat <<EOF > "$MACRO_FILE"
/run/numberOfThreads 1
/analysis/setFileName $OUTFILE
/run/initialize
/control/execute ../gun_electrons_testbeam.mac
/gps/pos/centre $X_START 0 $Z_GEN mm
/gps/direction $DX 0 $DZ
/run/beamOn $EVENTS
EOF

    echo "   -> L_target=${L}mm  X=${X_TARGET}mm  Th=${THETA}deg  X_START=${X_START}mm  -> $OUTFILE"
    nohup $EXECUTABLE $GEO "$MACRO_FILE" > "$LOG_FILE" 2>&1 &
    NJOBS=$((NJOBS + 1))

    while [ "$(jobs -p | wc -l)" -ge 8 ]; do
        sleep 1
    done
done

echo ""
echo "Lanciati $NJOBS job."
echo "Attendo completamento..."
wait
echo "FATTO. File scritti in ../data/"
