#!/bin/bash
# =======================================================
# Run_scan.sh — Produzione parallela con scan posizione/angolo
#
# Uso: bash Run_scan.sh <geo> <src>
#   geo:  single | triple
#   src:  e_tb | mu_tb | mu_lngs | gamma
#
# Naming output: sim_<geo>_<src>_X<x>mm_Y<y>mm_Th<theta>deg.root
# Output in: ../data/
#
# Sorgenti fascio (e_tb, mu_tb): scan su X, Y, Theta
# Sorgenti distribuite (mu_lngs, gamma): run singolo, nessun scan
# =======================================================

GEO=${1:-single}
SRC=${2:-e_tb}
EXECUTABLE="../build/WCtestbeamSim"
MAX_CORES=8
Z_GEN=2000.0   # mm — piano di generazione fascio

# =============================================
# PARAMETRI DI PRODUZIONE (modifica qui)
# =============================================
EVENTS=1000

# Scan posizione impatto sul detector (mm)
#X_POSITIONS=(-400 -300 -200 -100 0 100 200 300 400)
#Y_POSITIONS=(-140 -70 0 70 140)
X_POSITIONS=(0)
Y_POSITIONS=(0)

# Scan angolo fascio nel piano XZ (gradi)
ANGLES=(0)

# =============================================
# Mappa src → file GPS mac
# =============================================
case $SRC in
    e_tb)    GPS_MAC="gun_electrons_testbeam.mac" ;;
    mu_tb)   GPS_MAC="gun_muons_testbeam.mac"     ;;
    mu_lngs) GPS_MAC="gun_muons_lngs.mac"         ;;
    gamma)   GPS_MAC="gun_gammas_background.mac"  ;;
    *)
        echo "ERRORE: sorgente non valida '$SRC'"
        echo "       valori validi: e_tb | mu_tb | mu_lngs | gamma"
        exit 1
        ;;
esac

case $GEO in
    single|single_wbc|single_wbc2|single_wbc2_19f|single_wbc2_19fw|single_wbc3|triple) ;;
    *)
        echo "ERRORE: geometria non valida '$GEO'"
        echo "       valori validi: single | single_wbc | single_wbc2 | single_wbc2_19f | single_wbc2_19fw | triple"
        exit 1
        ;;
esac

mkdir -p ../data
mkdir -p tmp_jobs

echo "==================================================="
echo " PRODUZIONE WC Testbeam 2026"
echo " Geometria : $GEO"
echo " Sorgente  : $SRC  ($GPS_MAC)"
echo " Output    : ../data/"
echo "==================================================="

# =============================================
# Sorgenti distribuite: singolo run, nessun scan
# =============================================
if [ "$SRC" = "mu_lngs" ] || [ "$SRC" = "gamma" ]; then
    OUTFILE="../data/sim_${GEO}_${SRC}_X0mm_Y0mm_Th0deg.root"
    MACRO_FILE="tmp_jobs/run_${GEO}_${SRC}.mac"
    LOG_FILE="tmp_jobs/log_${GEO}_${SRC}.txt"

    cat <<EOF > "$MACRO_FILE"
/run/numberOfThreads 4
/analysis/setFileName $OUTFILE
/run/initialize
/control/execute ../$GPS_MAC
/run/beamOn $EVENTS
EOF

    echo " -> $OUTFILE"
    nohup $EXECUTABLE $GEO "$MACRO_FILE" > "$LOG_FILE" 2>&1 &
    wait
    echo "FATTO."
    exit 0
fi

# =============================================
# Sorgenti fascio: scan X × Y × Theta
# =============================================
NJOBS=0
for THETA in "${ANGLES[@]}"; do

    DX=$(awk -v t="$THETA" 'BEGIN{printf "%.6f", sin(t*3.14159265358979/180)}')
    DY=0.0
    DZ=$(awk -v t="$THETA" 'BEGIN{printf "%.6f", -cos(t*3.14159265358979/180)}')
    THETA_STR="$THETA"

    for Y in "${Y_POSITIONS[@]}"; do
        Y_STR=$(echo "$Y" | sed 's/-/m/')

        for X in "${X_POSITIONS[@]}"; do
            X_STR=$(echo "$X" | sed 's/-/m/')

            # Posizione di partenza a Z=Z_GEN tale che il fascio
            # colpisca il punto (X, Y, ~0) al centro del detector.
            # Formula: X_START = X + Z_GEN * (DX/DZ)
            # (DZ < 0 per fascio verso -Z, quindi X_START < X per theta > 0)
            X_START=$(awk -v x="$X" -v z="$Z_GEN" -v dx="$DX" -v dz="$DZ" \
                'BEGIN{printf "%.2f", x + z*(dx/dz)}')
            Y_START="$Y"

            OUTFILE="../data/sim_${GEO}_${SRC}_X${X_STR}mm_Y${Y_STR}mm_Th${THETA_STR}deg.root"
            MACRO_FILE="tmp_jobs/run_${GEO}_${SRC}_X${X_STR}_Y${Y_STR}_Th${THETA_STR}.mac"
            LOG_FILE="tmp_jobs/log_${GEO}_${SRC}_X${X_STR}_Y${Y_STR}_Th${THETA_STR}.txt"

            cat <<EOF > "$MACRO_FILE"
/run/numberOfThreads 1
/analysis/setFileName $OUTFILE
/run/initialize
/control/execute ../$GPS_MAC
/gps/pos/centre $X_START $Y_START $Z_GEN mm
/gps/direction $DX $DY $DZ
/run/beamOn $EVENTS
EOF

            echo "   -> geo=$GEO src=$SRC X=${X}mm Y=${Y}mm Th=${THETA}deg  |  start=(${X_START}, ${Y_START}, ${Z_GEN})"
            nohup $EXECUTABLE $GEO "$MACRO_FILE" > "$LOG_FILE" 2>&1 &
            NJOBS=$((NJOBS + 1))

            # Limita i job paralleli
            while [ "$(jobs -p | wc -l)" -ge "$MAX_CORES" ]; do
                sleep 1
            done
        done
    done
done

echo ""
echo "Lanciati $NJOBS job (max $MAX_CORES in parallelo)."
echo "Attendo completamento..."
wait
echo "FATTO. File scritti in ../data/"
