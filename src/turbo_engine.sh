#!/bin/bash

declare -A DRIVERS=(
    [acpi-cpufreq]="/sys/devices/system/cpu/cpufreq/boost"
    [amd-pstate]="/sys/devices/system/cpu/cpufreq/boost"
    [intel_cpufreq]="/sys/devices/system/cpu/intel_pstate/no_turbo"
    [intel_pstate]="/sys/devices/system/cpu/intel_pstate/no_turbo"
)

SCALING_DRIVER=$(</sys/devices/system/cpu/cpu0/cpufreq/scaling_driver)
if [ $SCALING_DRIVER = intel_cpufreq ] || [ $SCALING_DRIVER = intel_pstate ]; then
    TURBO_DISABLED="1"
    TURBO_ENABLED="0"
else
    TURBO_DISABLED="0"
    TURBO_ENABLED="1"
fi

#CPU_MAX_FREQ=$(</sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq)
BOOST_INTERFACE_PATH=${DRIVERS[$SCALING_DRIVER]}
ACTUAL_STATE=$(<${BOOST_INTERFACE_PATH})
APPLICATION_PROCESS_ID=$1

if [ -z "$IPC_TARGET" ]; then
    echo "TBFT/Urano - The environment variable IPC_TARGET is not set."
    echo "TBFT/Urano - Using default value: 0.5"
    IPC_TARGET=0.5
fi

if [ -z "$TIME_TARGET" ]; then
    echo "TBFT/Urano - The environment variable TIME_TARGET is not set."
    echo "TBFT/Urano - Using default value: 1"
    TIME_TARGET=1
fi

perf stat -a -e instructions,cycles -o out.txt sleep 1.0
while test -d /proc/${APPLICATION_PROCESS_ID}/; do
    sed -i 's/,/./g' out.txt
    IPC=$(egrep "insn" out.txt | awk '{print $4}')
    if [ $(echo "$IPC>$IPC_TARGET" | bc) -eq 0 ] && [ $ACTUAL_STATE -eq $TURBO_ENABLED ]; then
        ACTUAL_STATE=$TURBO_DISABLED
        echo $TURBO_DISABLED >${BOOST_INTERFACE_PATH}

    elif [ $(echo "$IPC<=$IPC_TARGET" | bc) -eq 0 ] && [ $ACTUAL_STATE -eq $TURBO_DISABLED ]; then
        ACTUAL_STATE=$TURBO_ENABLED
        echo $TURBO_ENABLED >${BOOST_INTERFACE_PATH}
    fi
    perf stat -a -e instructions,cycles -o out.txt sleep $TIME_TARGET
done