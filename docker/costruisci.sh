#!/bin/bash
# =======================================================
# costruisci.sh — costruisce l'immagine della simulazione e la carica su
# Docker Hub, per Bouchet (linux/amd64).
#
# Uso, dalla radice del repository o da docker/ (Docker Desktop acceso,
# `docker login` gia' fatto una volta):
#   bash docker/costruisci.sh <utente_dockerhub> [tag]
#     tag  default: il commit corrente (es. 28c2c05), piu' "-mod" se ci sono
#          modifiche non committate in src/, include/, sim.cc o nelle macro
#
# Carica <utente>/wcsim:<tag> e <utente>/wcsim:latest.
# ATTENZIONE: nell'immagine va la cartella cosi' com'e' sul disco, comprese
# le modifiche non committate. Il tag "-mod" serve a ricordarlo.
# Variabile facoltativa: PIATTAFORME (default linux/amd64; per provarla
# anche sul Mac: PIATTAFORME=linux/amd64,linux/arm64).
# =======================================================
set -eu
UTENTE=${1:?utente di Docker Hub}
RADICE=$(cd "$(dirname "$0")/.." && pwd)
cd "$RADICE"
COMMIT=$(git rev-parse --short HEAD)
SPORCO=""
git diff --quiet HEAD -- src include sim.cc CMakeLists.txt '*.mac' || SPORCO="-mod"
TAG=${2:-$COMMIT$SPORCO}
PIATTAFORME=${PIATTAFORME:-linux/amd64}
[ -n "$SPORCO" ] && echo "Nota: modifiche non committate, entrano nell'immagine (tag $TAG)."
echo "Costruisco $UTENTE/wcsim:$TAG per $PIATTAFORME, commit $COMMIT$SPORCO"
docker buildx build --platform "$PIATTAFORME" \
    --build-arg GIT_COMMIT="$COMMIT$SPORCO" \
    -t "$UTENTE/wcsim:$TAG" -t "$UTENTE/wcsim:latest" \
    --push .
echo
echo "Fatto. Su Bouchet:"
echo "  apptainer pull wcsim_$TAG.sif docker://$UTENTE/wcsim:$TAG"
