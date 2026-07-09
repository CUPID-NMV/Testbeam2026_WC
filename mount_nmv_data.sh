#!/usr/bin/env bash

# Ensure the official rclone / nc are found even under launchd's minimal PATH.
export PATH="/usr/local/bin:/opt/homebrew/bin:${PATH}"

# --- Configuration ---------------------------------------------------------
LOCAL_DIR="${HOME}/nmv-data"
REMOTE_NAME="mnv-data"
# Resolve rclone.conf relative to this script, not the current working directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RCLONE_CONF="${SCRIPT_DIR}/rclone.conf"
LOG_FILE="${HOME}/.nmv-data-mount.log"

# S3 endpoint probed before mounting (see network precheck below)
NET_HOST="s3.cr.cnaf.infn.it"
NET_PORT="7480"
NET_WAIT_TRIES=30     # number of probe attempts ...
NET_WAIT_GAP=4        # ... every NET_WAIT_GAP seconds (~2 min total)

# --- Mode ------------------------------------------------------------------
# --foreground : run rclone in the foreground (for launchd, which manages the
#                process lifetime). Default: background with nohup (interactive).
FOREGROUND=0
[ "$1" = "--foreground" ] && FOREGROUND=1

# --- Preconditions ---------------------------------------------------------
if ! command -v rclone >/dev/null 2>&1; then
    echo "Error: rclone is not installed or not in PATH." >&2
    exit 1
fi

if [ ! -f "${RCLONE_CONF}" ]; then
    echo "Error: rclone configuration file not found: ${RCLONE_CONF}" >&2
    exit 1
fi

if [ ! -d "${LOCAL_DIR}" ]; then
    echo "Creating directory ${LOCAL_DIR}"
    mkdir -p "${LOCAL_DIR}" || {
        echo "Error: unable to create ${LOCAL_DIR}" >&2
        exit 1
    }
fi

# Already mounted? (macOS/Linux compatible)
if mount | grep -qF " on ${LOCAL_DIR} "; then
    echo "Directory ${LOCAL_DIR} is already a mountpoint. Nothing to do."
    exit 0
fi

# --- Network precheck ------------------------------------------------------
# rclone mount would attach even with no connectivity, but then every access to
# the mountpoint HANGS. Wait for the S3 endpoint; if it never answers, do NOT
# mount (avoids a hung/poisoned mountpoint at login when offline).
echo "Waiting for ${NET_HOST}:${NET_PORT} to be reachable ..."
_net_ok=0
for _t in $(seq 1 "${NET_WAIT_TRIES}"); do
    if nc -z -G 3 "${NET_HOST}" "${NET_PORT}" 2>/dev/null; then
        _net_ok=1
        break
    fi
    sleep "${NET_WAIT_GAP}"
done
if [ "${_net_ok}" -ne 1 ]; then
    echo "No connectivity to ${NET_HOST}:${NET_PORT} — skip mount." >&2
    exit 0
fi

# --- Mount -----------------------------------------------------------------
# On macOS 'rclone mount --daemon' is unreliable with macFUSE (it times out and
# can leave a stale/poisoned mountpoint), so we never use --daemon.
echo "Mounting ${REMOTE_NAME}: on ${LOCAL_DIR} using ${RCLONE_CONF}..."

if [ "${FOREGROUND}" -eq 1 ]; then
    # launchd owns this process: run rclone in the foreground.
    exec rclone mount "${REMOTE_NAME}:" "${LOCAL_DIR}" \
        --config="${RCLONE_CONF}" \
        --vfs-cache-mode off
fi

# Interactive: background the mount, then confirm it actually attached.
nohup rclone mount "${REMOTE_NAME}:" "${LOCAL_DIR}" \
    --config="${RCLONE_CONF}" \
    --vfs-cache-mode off \
    > "${LOG_FILE}" 2>&1 &
disown

for _i in $(seq 1 15); do
    if mount | grep -qF " on ${LOCAL_DIR} "; then
        echo "Mount completed successfully."
        exit 0
    fi
    sleep 1
done

echo "Error: mount did not appear within timeout. Last log lines:" >&2
tail -n 5 "${LOG_FILE}" >&2
exit 1
