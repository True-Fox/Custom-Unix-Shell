all: compile run

compile:
	g++ -std=c++20 main.cpp

run:
	./a.out

clean:
	rm ./a.out
