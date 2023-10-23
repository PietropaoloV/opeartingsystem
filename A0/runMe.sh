#!/bin/bash

current_directory=$(pwd)

if [[ "${current_directory: -2}" == "A0" ]]; then
    echo true
else
    echo "Please cd into A0 directory"
    exit 1
fi


make clean
make
cd benchmarks
make clean
make

echo "***********************Running Vector Multiply with RR***********************"
./vector_multiply 6 0
echo "***********************Running Vector Multiply with PSJF***********************"
./vector_multiply 6 1
echo "***********************Running Vector Multiply with MLFQ***********************"
./vector_multiply 6 2

echo "***********************Running Parallel Cal with RR***********************"
./parallel_cal 6 0
echo "***********************Running Parallel Cal with PSJF***********************"
./parallel_cal 6 1
echo "***********************Running Parallel Cal with MLFQ***********************"
./parallel_cal 6 2

echo "***********************Running External Cal with RR***********************"
./external_cal 6 0
echo "***********************Running External Cal with PSJF***********************"
./external_cal 6 1
echo "***********************Running External Cal with MLFQ***********************"
./external_cal 6 2


echo "***********************Running Benchmark 1 with RR***********************"
./bm1 0
echo "***********************Running Benchmark 1 with PSJF***********************"
./bm1 1
echo "***********************Running Benchmark 1 with MLFQ***********************"
./bm1 2

echo "***********************Running Benchmark 2 with RR***********************"
./bm2 100 0
echo "***********************Running Benchmark 2 with PSJF***********************"
./bm2 100 1
echo "***********************Running Benchmark 2 with MLFQ***********************"
./bm2 100 2


echo "***********************Running Benchmark 3 with RR***********************"
./bm3 100 0
echo "***********************Running Benchmark 3 with PSJF***********************"
./bm3 100 1
echo "***********************Running Benchmark 3 with MLFQ***********************"
./bm3 100 2


echo "***********************Running Benchmark 4 with RR***********************"
./bm4 100 0
echo "***********************Running Benchmark 4 with PSJF***********************"
./bm4 100 1
echo "***********************Running Benchmark 4 with MLFQ***********************"
./bm4 100 2
