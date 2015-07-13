#!/bin/bash

rm -rf *.dol
rm -rf *.elf
rm -rf *.gcm

rm -rf dist

make clean

make

elf2dol swiss.elf swiss.dol
d0lz swiss.dol swiss-lz.dol -m
d0lz swiss.dol swiss-lz-viper.dol -m -v
mkdir dist
cp swiss-lz.dol dist
mv swiss.dol dist
mv swiss-lz-viper.dol dist
