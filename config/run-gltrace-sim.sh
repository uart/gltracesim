#!/bin/bash

#SBATCH -A project_name
#SBATCH -n 16
#SBATCH -t 3-0:00:00
#SBATCH --mail-type=FAIL
#SBATCH --mem=3G

source /etc/profile
source ${HOME}/local/conf.rc
source ${GLTRACE_SIM_HOME}/ext/conf.rc

echo "$(date): Simulation started."

${XVFB_RUN} -s "${XVFB_ARGS}" ${XVFB_RUN_ARGS} \
    ${GLTRACE_SIM_HOME}/${GLTRACE_SIM_BIN} --pin-flags='-ifeellucky'\
    -i ${TRACE_FILE} -o ${OUT_DIR} --num-analysis-threads 1 --stop-time 169200 \
    -f ${F} -w ${W} -n 16 -d Init,GpuFrameEvent

echo "$(date): Simulation completed."