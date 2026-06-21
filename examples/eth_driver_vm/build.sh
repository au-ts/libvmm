#!/bin/sh
# build.sh
exec make \
  MICROKIT_BOARD=odroidc4 \
  MICROKIT_SDK="$HOME/thesis/microkit-sdk-2.2.0" \
  MICROKIT_CONFIG=benchmark \
  BENCH_PMU_EVENTS=EXECUTE_INSTRUCTION,CHAIN,MEM_ACCESS,CHAIN,CACHE_L1D_MISS,CHAIN \
  LINUX="$HOME/thesis/linux/arch/arm64/boot/Image" \
  "$@"