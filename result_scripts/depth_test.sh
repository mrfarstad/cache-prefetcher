#!/bin/bash
mkdir depth_stats
mkdir depth_stats/depth_2
sizes=(4096 2048 1024 512 256 128 64 32)

sed -i -re 's/(#define DEPTH) [0-9]+/\1 '$j'/' src/prefetcher.cc
sed -i -re 's/(#define WIDTH) [0-9]+/\1 '$j'/' src/prefetcher.cc

for i in "${sizes[@]}"
do
  :
  mkdir depth_stats/depth_2/size_$i
  sed -i -re 's/(#define GHB_SIZE) [0-9]+/\1 '$i'/; s/(#define AIT_SIZE) [0-9]+/\1 '$i'/' src/prefetcher.cc
  echo "Size: $i"
  make
  mv stats.txt depth_stats/depth_2/size_$i
done
