#!/bin/bash
# =======================================================
# scarica_dataset.sh — scarica i dataset di Geant4 nell'immagine, al posto
# di `geant4-config --install-datasets`, che a una connessione caduta
# butta via tutto (successo il 2026-11-11 con G4NDL all'89%).
# Qui ogni file riprende da dove si era fermato (curl -C -), fino a 30
# tentativi, e si controlla l'MD5 che Geant4 dichiara.
#
# Uso (lo chiama il Dockerfile): scarica_dataset.sh [DA_SALTARE]
#   DA_SALTARE  nomi separati da virgole, es. "G4NDL,G4CHANNELING": al loro
#               posto una cartella vuota. G4NDL (1 GB) serve solo alla
#               fisica dei neutroni HP (es. QGSP_BERT_HP), che la physics
#               list del WC non usa.
# Variabile facoltativa: G4DATA_URL, un mirror al posto di cern.ch.
# =======================================================
set -euo pipefail
SALTA=${1:-}
URL=${G4DATA_URL:-https://cern.ch/geant4-data/datasets}
CONFIG=$(command -v geant4-config)

# Elenco nome|variabile|cartella|file|md5, scritto in geant4-config dalla build
LISTA=$(grep -o 'split( "[^"]*"' "$CONFIG" | head -1 | sed 's/^split( "//; s/"$//' | tr ';' '\n')
[ -n "$LISTA" ] || { echo "non trovo l'elenco dei dataset in $CONFIG"; exit 1; }

TMP=$(mktemp -d)
while IFS='|' read -r nome var dir file md5; do
  [ -n "$nome" ] || continue
  if [[ ",$SALTA," == *",$nome,"* ]]; then
    mkdir -p "$dir"; echo "= $nome saltato (cartella vuota $dir)"; continue
  fi
  if [ -d "$dir" ] && [ -n "$(ls -A "$dir")" ]; then
    echo "= $nome gia' presente"; continue
  fi
  echo "+ $nome: $file"
  ok=0
  for t in $(seq 1 30); do
    rc=0
    curl -sS -L -f -C - --connect-timeout 30 -o "$TMP/$file" "$URL/$file" || rc=$?
    [ "$rc" = 0 ] && { ok=1; break; }
    # 33: il server non riprende da meta' file -> si riparte da zero
    [ "$rc" = 33 ] && rm -f "$TMP/$file"
    echo "  tentativo $t non riuscito (curl $rc), riprendo fra 10 s"; sleep 10
  done
  [ "$ok" = 1 ] || { echo "- $file: download fallito"; exit 1; }
  echo "$md5  $TMP/$file" | md5sum -c --quiet - || { echo "- $file: MD5 sbagliato"; exit 1; }
  mkdir -p "$(dirname "$dir")"
  tar xzf "$TMP/$file" -C "$(dirname "$dir")"
  rm -f "$TMP/$file"
  [ -d "$dir" ] || { echo "- $file non contiene $dir"; exit 1; }
  echo "  $nome in $dir ($(du -sh "$dir" | cut -f1))"
done <<< "$LISTA"
rm -rf "$TMP"
echo "dataset pronti"
