#!/bin/bash
# Uso: bash Run_MC.sh [single|triple]
# Default: triple

EXECUTABLE="../build/WCtestbeamSim"
GEO=${1:-triple}   # "single" o "triple"
EVENTS=1000
MAX_CORES=8

Z_FIXED=2000.0
ANGLES=(0)

if [ "$GEO" = "single" ]; then
    X_POSITIONS=$(seq -400 200 400)
    Y_POSITIONS=(0.0)
else
    X_POSITIONS=$(seq -400 200 400)
    Y_POSITIONS=(145.0 75.0 0.0 -75.0 -145.0)
fi

mkdir -p ../data
mkdir -p tmp_jobs

for THETA_DEG in "${ANGLES[@]}"; do

    DX=$(awk -v t=$THETA_DEG 'BEGIN{print  sin(t*3.14159265/180)}')
    DY=0.0
    DZ=$(awk -v t=$THETA_DEG 'BEGIN{print -cos(t*3.14159265/180)}')

    echo "==================================================="
    echo " PRODUZIONE [${GEO}] - FASCIO ${THETA_DEG} deg"
    echo " Direzione: ($DX, $DY, $DZ)"
    echo "==================================================="

    for Y in "${Y_POSITIONS[@]}"; do
        for X in $X_POSITIONS; do

            X_START=$(awk -v x=$X -v z=$Z_FIXED -v dx=$DX -v dz=$DZ 'BEGIN{print x - z*dx/dz}')
            Y_START=$Y

            FILENAME="../data/sim_${GEO}_Z2000_Y${Y}_X${X}_${THETA_DEG}Deg.root"
            MACRO_FILE="tmp_jobs/run_${GEO}_Y${Y}_X${X}_${THETA_DEG}Deg.mac"
            LOG_FILE="tmp_jobs/log_${GEO}_Y${Y}_X${X}_${THETA_DEG}Deg.txt"

            cat <<EOF > $MACRO_FILE
/run/numberOfThreads 1

/analysis/setFileName $FILENAME

/run/initialize

/gun/position $X_START $Y_START $Z_FIXED mm
/gun/direction $DX $DY $DZ

/run/beamOn $EVENTS
EOF

            echo "   -> Impatto (X=$X, Y=$Y) | Angolo=${THETA_DEG}°"
            nohup $EXECUTABLE $GEO $MACRO_FILE > $LOG_FILE 2>&1 &

            while [ $(jobs -p | wc -l) -ge $MAX_CORES ]; do
                sleep 1
            done
        done
    done
done

echo "Attendo completamento..."
wait
echo "FATTO."
