# =======================================================
# Immagine della simulazione WC + pannelli (WCtestbeamSim), per la
# produzione su Bouchet (Apptainer + SLURM).
#
# Base: carlomt/geant4, Geant4 gia' compilato (Ubuntu 22.04, amd64 e arm64),
# senza Qt/OpenGL: va bene in batch. Stessa versione della build sul Mac.
# I dataset di Geant4 non sono nella base: li scarica il primo RUN da cern.ch
# (~2 GB, una volta sola: il passo resta in cache finche' non cambia la
# versione). Il codice si copia dopo, cosi' a ogni modifica si ricompila
# solo la simulazione, in pochi minuti.
#
# Costruire e caricare su Docker Hub, dalla radice del repository, ramo
# pannelli (Bouchet e' x86_64: serve linux/amd64 anche dal Mac M4):
#   docker login
#   docker buildx build --platform linux/amd64 \
#       -t <utente_dockerhub>/wcsim:<tag> --push .
# Istruzioni complete: docker/LEGGIMI.md
# =======================================================
ARG G4_VERSION=11.4.2
FROM carlomt/geant4:${G4_VERSION}

# Dataset di Geant4 dentro l'immagine (nella cartella che la build della
# base ha registrato, /g4data): l'immagine non dipende da niente sul
# cluster. Lo script riprende i download interrotti e controlla l'MD5.
# G4NDL (1 GB, neutroni HP) e G4CHANNELING non servono alla physics list
# del WC: per averli, --build-arg DATASET_DA_SALTARE=""
ARG DATASET_DA_SALTARE=G4NDL,G4CHANNELING
# Mirror dei dataset al posto di cern.ch (vuoto = cern.ch)
ARG G4DATA_URL=
COPY docker/scarica_dataset.sh /usr/local/bin/
RUN G4DATA_URL="${G4DATA_URL:-https://cern.ch/geant4-data/datasets}" \
    bash /usr/local/bin/scarica_dataset.sh "${DATASET_DA_SALTARE}"

WORKDIR /opt/wcsim
COPY CMakeLists.txt sim.cc ./
COPY include/ include/
COPY src/ src/
COPY *.mac ./
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build -j"$(nproc)" \
 && rm -rf build/CMakeFiles

# Commit da cui e' stata costruita l'immagine (lo passa docker/costruisci.sh)
ARG GIT_COMMIT=sconosciuto
LABEL wcsim.git_commit=${GIT_COMMIT}
RUN echo "${GIT_COMMIT}" > /opt/wcsim/GIT_COMMIT

# Lanciatore `wcsim` (stessi argomenti di WCtestbeamSim): imposta le
# variabili dei dataset da geant4-config e avvia il programma. Funziona
# uguale con docker run e con apptainer exec, che non passa dall'entrypoint.
RUN printf '%s\n' '#!/bin/bash' \
      'eval "$(geant4-config --datasets | awk '"'"'{print "export " $2 "=" $3}'"'"')"' \
      'exec /opt/wcsim/build/WCtestbeamSim "$@"' > /usr/local/bin/wcsim \
 && chmod +x /usr/local/bin/wcsim

ENV PATH=/opt/wcsim/build:${PATH} \
    WCSIM_DIR=/opt/wcsim
# La base stampa un lungo messaggio a ogni avvio: qui niente entrypoint.
ENTRYPOINT []
CMD ["bash"]
