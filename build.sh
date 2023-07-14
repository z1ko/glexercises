#!/bin/bash

#sudo apt-get install libglfw3-dev libassimp-dev

for DIR in exercises/*; do
	mkdir -p $DIR/build && cmake -S $DIR -B $DIR/build && make -j -C $DIR/build
done
