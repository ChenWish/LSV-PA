#!/bin/bash

ITER=100
LSV_REDUCE=$(pwd)/lsv_reduce.dofile
ABC_DIR=../../../
BENCHMARK_DIR="$(pwd)/../../../../../IWLS/benchmarks"

if [ $# -ne 1 ]
then
    echo "usage: ./run.sh [input]" 
    exit
fi

mkdir results/$1
DIR=$(pwd)/results/$1
DOFILE=$DIR/dofile
LOGFILE=$DIR/log
INPUT_FILE=$BENCHMARK_DIR/$1

echo "read_truth -xf " $INPUT_FILE > $DOFILE
echo "strash" >> $DOFILE
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
    echo "source " $LSV_REDUCE >> $DOFILE
    echo "write_aiger " $DIR/iter$i.aig >> $DOFILE
done

cd $ABC_DIR
./abc < $DOFILE > $LOGFILE




