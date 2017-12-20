# SGEMM Benchmark for Software Defined Memory system, prepared by ScaleMP

#! /bin/bash

############# SETUP ############

# SEG is used to calculate the dimensions of the matrices participating in the calculation.
SEG=43211

# MEMORY_USAGE_PERCENTAGE defines the maximum RAM percentage that the workload can use. 
MEMORY_USAGE_PERCENTAGE=90

# Set the full path to Intel's compiler script. 
# If the code is not precompiled and statically linked to MKL, uncomment the line below and set the right path to Intel compiler
# INTEL_COMPILER_VARS=/opt/intel/bin/compilervars.sh

################################

function display_usage {

echo "Usage: run.sh [argoments]

The argument controls how many FAC values are tested. 
 
There are 4 options for arguments: 
	short:	Use two values for the test: 1 and the maximal value for fac that fits into 90% of system memory
	medium: Default option: from 1 to the maximal value by powers of 2. 
	full:   The entire range from 1 to the possible maximum is tested
	list:   Explicit (quoted) list is set by the user in the next parameter

The README file describes how the FAC values are calculated according to the the avilable RAM available
"
} 

# fac_list returns a lists with the various values for fac to test. 
function set_fac_list {

    if [ $# == 0 ] ; then
        # Set default	
        parameter="medium"
    else 
        parameter=$1
    fi

    max_fac=$(get_max_fac)

    reply=""  
    if [ $parameter == "short" ] ; then  
       reply="1 $max_fac"
    elif [ $parameter == "medium" ] ; then
        for (( i=1; i < $max_fac; i*=2 )); do
            reply="$reply $i"
        done
	reply="$reply $max_fac"
    elif [ $parameter == "full" ] ; then
        reply=$(seq 1 $max_fac)
    elif [ $parameter == "list" ] ; then
	shift
        reply=$@ 
    else 
        display_usage
        exit
    fi
	
    echo $reply

}	

# get_max_fac calculates the largest posisble factor to run on the system by available memory
function get_max_fac {
    seg=$SEG
    LOGFILE=setup.log
    memory_usage_percentage=$MEMORY_USAGE_PERCENTAGE
    
    #Calculate matices dimensions
    echo -e "matices dimensions calculations:" >> $LOGFILE
    mem_kb=`free   | grep Mem: | awk '{print $2}'`
    mem_gb=`free -g| grep Mem: | awk '{print $2}'`

    echo -e "Initial seg: \t" $seg  >> $LOGFILE
    echo -e "RAM \t\t $mem_gb GB \t $mem_kb KB" >> $LOGFILE
    
    mem_target_in_bytes=$(( mem_kb*1024*memory_usage_percentage/100 ))
    
    factor=$(echo "( -1+sqrt(4+$mem_target_in_bytes/$seg^2)/2 )" | bc -l )
    factor_int=$( echo $factor | awk -F. '{print $1}' )
    
    total_memory_size_kb=$(( (4 * $seg ** 2 * ( $factor_int ** 2 + 2 * $factor_int)) / 1024 ))
    total_memory_size_gb=$(echo "($total_memory_size_kb / 1024 / 1024  ) " | bc ) 

    echo -e "RAM alloc: \t $total_memory_size_gb GB \t $total_memory_size_kb KB" >> $LOGFILE
    
    mem_usage_percentage=$(echo "(($total_memory_size_kb * 100) / $mem_kb) " | bc -l | head -c 5 )

    echo -e "RAM usage: \t $mem_usage_percentage %" >> $LOGFILE
    echo -e "SEG: \t\t" $seg >> $LOGFILE
    echo -e "FAC: \t\t" $factor_int >> $LOGFILE
    echo >> $LOGFILE
    date >> $LOGFILE

    echo $factor_int
}    
    
############### MAIN ##########################################

for fname in `echo -n /usr/local/bin/????version | sed 's/^\(.*\)\(\/usr\/local\/bin\/vsmpversion\)\(.*\)$/\1\3 \2/'`; do
        strings $fname 2>&1 | grep -iq "vsmpversion"
        if [ $? == 0 ]; then TOOL=`basename $fname | awk '{print substr($0,0,4)}'`; break; fi
done

${TOOL}ctl --nna=on

if [ -n "$INTEL_COMPILER_VARS" ]; then
	. $INTEL_COMPILER_VARS intel64
	#. /opt/intel/bin/compilervars.sh intel64
     echo " using INTEL_COMPILER_VARS"
fi

ncpus=`grep -c processor /proc/cpuinfo`
export KMP_AFFINITY=explicit,proclist=[`seq -s , $((ncpus-1)) -1 0`],granularity=fine

TAG=`date +%y%m%d-%H%M%S`

rm -f setup.log
( echo "===>> $HOSTNAME system -"; uname -a; echo; free -g; echo; uptime; echo; grep ^ /sys/kernel/mm/transparent_hugepage/enabled; echo; lscpu; echo; if [ -x /usr/local/bin/${TOOL}version ]; then ${TOOL}version -vvv; ${TOOL}ctl --status; else echo 'Running NATIVE'; fi; echo ) > setup.log 2>&1

for x in $((ncpus/2)) ; do
  export OMP_NUM_THREADS=$x

  FAC=$(set_fac_list $@)

echo "The following FAC values will be tested starting now: $FAC"  

date

  for seg in $SEG ; do
    for factor in $FAC ; do
      for size in $(( factor * seg )) ; do
        # By default we run one test cycle per size. 
        for cycle in 1 ; do
          cp setup.log log-gemm-$factor-$x-$size-$seg-seg-ilv-$cycle.$TAG.txt
          numactl --interleave all /usr/bin/time -p ./mkl-seg $size $size $seg $seg $seg $seg 2>&1 | tee -a log-gemm-$factor-$x-$size-$seg-seg-ilv-$cycle.$TAG.txt
        done
      done
    done
  done
done

rm -f setup.log

