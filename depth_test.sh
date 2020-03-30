#!/bin/bash
mkdir stats
mkdir stats/gdc_depth
widths=(2 4 8 16)
depths=(2 4 8 16)

sed -i -re 's/(#define GHB_SIZE) [0-9]+/\1 512/; s/(#define AIT_SIZE) [0-9]+/\1 512/' src/prefetcher.cc
for j in "${depths[@]}"
do
  :
  mkdir stats/gdc_depth/depth_$j
  sed -i -re 's/(#define DEPTH) [0-9]+/\1 '$j'/' src/prefetcher.cc
  for i in "${widths[@]}"
  do
    :
    sed -i -re 's/(#define WIDTH) [0-9]+/\1 '$j'/' src/prefetcher.cc
    mkdir stats/gdc_depth/depth_$j/width_$i
    echo "$j $i"
    make
    mv stats.txt stats/gdc_depth/depth_$j/width_$i
  done
done
