#!/bin/sh

make clean all

rmmod msp3520fb
insmod msp3520fb.ko