all: sample2D

sample2D: main.cpp
	g++ -g -o sample2D main.cpp -lglfw -lGLEW -lGL -ldl

clean:
	rm sample2D
