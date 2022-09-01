#!/bin/bash

declare -A DRIVERS=(
        [acpi-cpufreq]="/sys/devices/system/cpu/cpufreq/boost"
        [intel_cpufreq]="/sys/devices/system/cpu/intel_pstate/no_turbo"
        [intel_pstate]="/sys/devices/system/cpu/intel_pstate/no_turbo"
)

SCALING_DRIVER=$(</sys/devices/system/cpu/cpu0/cpufreq/scaling_driver)
if [ $SCALING_DRIVER = intel_cpufreq ] || [ $SCALING_DRIVER = intel_pstate ]
then
        TURBO_DISABLED="1"
        TURBO_ENABLED="0"
else
        TURBO_DISABLED="0"
        TURBO_ENABLED="1"
fi

ACTUAL_STATE=$(<${DRIVERS[$SCALING_DRIVER]})
perf stat -e instructions,cycles -o out.txt sleep 1.0

while test -d /proc/$1/
do
        sed -i 's/,/./g' out.txt
        ipc=$(egrep "insn" out.txt | awk '{print $4}')
        if [ $(echo "$ipc>$IPC_TARGET"| bc) -eq 0 ] && [ $ACTUAL_STATE -eq $TURBO_ENABLED ] ;
        then
                ACTUAL_STATE=$TURBO_DISABLED
                echo $TURBO_DISABLED > ${DRIVERS[$SCALING_DRIVER]}

        elif [ $(echo "$ipc<=$IPC_TARGET"| bc) -eq 0 ] && [ $ACTUAL_STATE -eq $TURBO_DISABLED ];
        then
                ACTUAL_STATE=$TURBO_ENABLED
                echo $TURBO_ENABLED > ${DRIVERS[$SCALING_DRIVER]}
        fi
        perf stat -e instructions,cycles -o out.txt sleep $TIME_TARGET
done