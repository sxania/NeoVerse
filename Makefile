# NeoVerse: AI City Survival System
# Makefile for POSIX / MSYS2 / MinGW / WSL. For Visual Studio open neoverse cpp.slnx.
#
#   make          build ./neoverse
#   make run      build, then start the simulation
#   make clean    remove the executable and any object files
#   make rebuild  clean + build

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
TARGET   := neoverse

# Every translation unit in the project (kept in sync with neoverse cpp.vcxproj).
SRCS := main.cpp \
        Analytics.cpp \
        CityComponent.cpp \
        CityData.cpp \
        Engineer.cpp \
        Event.cpp \
        EventManager.cpp \
        FileManager.cpp \
        Utils.cpp

OBJS := $(SRCS:.cpp=.o)
DEPS := Analytics.h CityComponent.h CityData.h Config.h Engineer.h Event.h \
        EventManager.h FileManager.h Utils.h

# Delete command: `rm -f` on POSIX shells (Linux/macOS/WSL/MSYS2/Git Bash),
# `del` on Windows cmd/PowerShell, so `make clean` never fails on either.
ifeq ($(OS),Windows_NT)
    RM := del /f /q 2>nul
else
    RM := rm -f
endif

.PHONY: all run clean rebuild

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET) $(TARGET).exe $(OBJS)

rebuild: clean all
