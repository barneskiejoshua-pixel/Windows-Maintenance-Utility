# Makefile for the Windows System Maintenance Utility (NPARC 2.6).
#
# Targets:
#   make            Build maintenance.exe with MinGW / GCC (release flags).
#   make clean      Remove the built executable.
#
# This GNU makefile targets MinGW. Visual Studio users build instead with:
#   cl /MT /O2 maintenance.cpp /Fe:maintenance.exe

CXX      = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra
LDFLAGS  = -static -static-libgcc -static-libstdc++ -s
TARGET   = maintenance.exe
SRC      = maintenance.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

.PHONY: clean
clean:
	-del /q $(TARGET)
