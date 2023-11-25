#!/bin/sh

set -xe

CC="${CXX:-cc}"

$CC sim.c -o sim -Wall -ggdb -O3 -std=c11 -pedantic
