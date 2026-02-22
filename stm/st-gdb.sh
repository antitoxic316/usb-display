#!/bin/bash

GDB_SERVER_BIN=/home/dima/st/stm32cubeclt_1.20.0_2/STLink-gdb-server/bin/ST-LINK_gdbserver

exec ${GDB_SERVER_BIN} -c STLINK-gdb-server.conf