#!/bin/bash

ITER=200
LSV_REDUCE=$(pwd)/lsv_reduce.dofile
REDUCE_DC=$(pwd)/reduce_dc.dofile
REDUCE_SMART=$(pwd)/reduce_smart.dofile
ABC_DIR=../../../
BENCHMARK_DIR="$(pwd)/../../../../../IWLS/benchmarks"

if [ $# -ne 1 ]
then
    echo "usage: ./run.sh [input]" 
    exit
fi

mkdir results/Collapse_$1
DIR=$(pwd)/results/Collapse_$1
DOFILE=$DIR/dofile
LOGFILE=$DIR/log
INPUT_FILE=$BENCHMARK_DIR/$1

echo "read_truth -xf " $INPUT_FILE > $DOFILE
echo "collapse" >> $DOFILE
echo "sop" >> $DOFILE
echo "strash" >> $DOFILE
echo "dc2" >> $DOFILE
echo "ps" >> $DOFILE

for i in {0..5}
do
    echo "resyn" >> $DOFILE
    echo "resyn2" >> $DOFILE
    echo "resyn3" >> $DOFILE
done

echo "ps" >> $DOFILE



for ((i=0; i<$ITER; ++i ))
do
    echo "lsv_chain_time_limit 100" >> $DOFILE
    echo "lsv_chain_dc_bound " $((9+$i/20)) " 4" >> $DOFILE
    echo "lsv_chain_bad_root_clear" >> $DOFILE
    echo "source " $REDUCE_DC >> $DOFILE
    echo "write_aiger " $DIR/dc_iter$i.aig >> $DOFILE

    echo "lsv_chain_time_limit 100" >> $DOFILE
    echo "lsv_chain_dc_bound " $((5+$i/25)) " 4" >> $DOFILE
    echo "lsv_chain_bad_root_clear" >> $DOFILE
    echo "source " $REDUCE_SMART >> $DOFILE
    echo "write_aiger " $DIR/smart_iter$i.aig >> $DOFILE
done

cd $ABC_DIR
./abc < $DOFILE > $LOGFILE




