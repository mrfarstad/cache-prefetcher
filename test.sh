#!/bin/bash
mkdir stats/gdc
#depths=(2 4 8)
#ghb_sizes=(16 32 64 128 256 512 1024)
depths=(2)
ghb_sizes=(16 32)

#reset prefetch depth
for j in "${depths[@]}"
do
  :
  sed -i -re 's/(#define DEPTH) [0-9]+/\1 '$j'/' src/prefetcher.cc
  for i in "${ghb_sizes[@]}"
  do
    :
    sed -i -re 's/(#define GHB_SIZE) [0-9]+/\1 '$i'/; s/(#define AIT_SIZE) [0-9]+/\1 '$i'/' src/prefetcher.cc
    mkdir stats/gdc/ghb_$i
    make
    mv stats_* stats/gdc/ghb_$i
  done
done


