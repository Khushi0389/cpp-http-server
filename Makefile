CXX = g++
CXXFLAGS = -Wall -std=c++11
LDFLAGS = -lz
SRC = src/main.cpp src/server.cpp
OUT = server

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)
