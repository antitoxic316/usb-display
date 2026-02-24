STL=st-flash

BIN_PATH=build/cubemx.bin

make clean
make
${STL} write ${BIN_PATH} 0x8000000
