CXX = g++
OBJ = build/simba.o build/sir.o build/sasm.o

all: build/simba.o build/sir.o build/sasm.o

build/%.o: src/%.cpp
	$(CXX) $< -o $@

clean:
	rm -f $(OBJ)
