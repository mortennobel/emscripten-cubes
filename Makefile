all: spin

cubes: main.o
	clang++ -framework SDL2_image -framework SDL2 -framework OpenGL main.o -o cubes

main.o: main.cpp
	clang++ -std=gnu++11 -stdlib=libc++ -c main.cpp

clean:
	rm -rf *o spin
