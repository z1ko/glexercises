#!/bin/bash

gcc main.cpp -ldl -lglfw3 -lassimp -I../vendor/ -I../lib/ -o gbuffer
