include Make_linux.inc
#include Make_msys2.inc
#include Make_osx.inc

CXXFLAGS = -std=c++17
ifdef DEBUG
CXXFLAGS += -g -O0 -Wall -fbounds-check -pedantic -D_GLIBCXX_DEBUG
CXXFLAGS2 := $(CXXFLAGS)
else
CXXFLAGS2 := $(CXXFLAGS) -O2 -march=native -Wall
CXXFLAGS += -O3 -march=native -Wall
endif

ALL = ../build/ant_simu.exe ../build/ant_simu_vect.exe

default: help

all: $(ALL)

../build:
	mkdir -p ../build
clean:
	@rm -fr ../build/*.o ../build/*.exe ../build/*~
../build/%.o: %.cpp | ../build
	$(CXX) $(CXXFLAGS2) -c $< -o $@
../build/ant_simu.exe: ../build/ant.o ../build/fractal_land.o ../build/renderer.o ../build/window.o ../build/ant_simu.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)
../build/ant_simu_vect.exe: ../build/ant_vect.o ../build/fractal_land.o ../build/renderer_vect.o ../build/window.o ../build/ant_simu_vect.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)

help:
	@echo "Available targets : "
	@echo "    all            : compile all executables"
	@echo "Add DEBUG=yes to compile in debug"
	@echo "Configuration :"
	@echo "    CXX      :    $(CXX)"
	@echo "    CXXFLAGS :    $(CXXFLAGS)"
%.html: %.md
	pandoc -s --toc $< --css=./github-pandoc.css --metadata pagetitle="OS202 - TD1" -o $@