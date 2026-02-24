#!/bin/bash

ST_CLT_PREFIX=/home/dima/st/stm32cubeclt_1.20.0_2
GDB=${ST_CLT_PREFIX}/GNU-tools-for-STM32/bin/arm-none-eabi-gdb
STL=st-flash

BIN_PATH=build/cubemx.bin
ELF_PATH=build/cubemx.elf

#export CFLAGS="$CFLAGS -Wl,-Map -Wl,mapfile -Wl,--cref"

make clean
make
${STL} write ${BIN_PATH} 0x8000000
${GDB} ${ELF_PATH} -ex "target extended-remote localhost:61234"
