all: main.out

main.out: main.o
	g++ -o $@ $^ -L$(stdOpenGL) -lshader_program -lGL -lglfw -ldl

%.o: %.cpp
	g++ -o $@ $^ -c -I $(stdOpenGL)
