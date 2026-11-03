CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
OUT      ?= ../build/solpkg.exe

all: $(OUT)

$(OUT): main.cpp sha256.hpp tar.hpp control.hpp
	@mkdir -p $(dir $(OUT))
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

clean:
	rm -f $(OUT)

.PHONY: all clean