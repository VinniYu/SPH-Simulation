# Common flags
LDFLAGS_COMMON = -lGLEW -lGL -lGLU -lglut -lstdc++
CFLAGS_COMMON  = -c -Wall -I./ -O3 -DGL_SILENCE_DEPRECATION

# Compiler
CC      = g++
CFLAGS  = ${CFLAGS_COMMON}
LDFLAGS = ${LDFLAGS_COMMON}

# Executable names
EXECUTABLE_2 = 2D_SPH
EXECUTABLE_3 = 3D_SPH
EXECUTABLES  = $(EXECUTABLE_2) $(EXECUTABLE_3)

# Source files
SOURCES_2D = 2D_SPH.cpp PARTICLE_2D.cpp SHADER.cpp
SOURCES_3D = 3D_SPH.cpp PARTICLE_3D.cpp SHADER.cpp

OBJECTS_2D = $(SOURCES_2D:.cpp=.o)
OBJECTS_3D = $(SOURCES_3D:.cpp=.o)

# Default target
all: $(EXECUTABLES)

# Build rules
$(EXECUTABLE_2): $(OBJECTS_2D)
	$(CC) $(OBJECTS_2D) $(LDFLAGS) -o $@

$(EXECUTABLE_3): $(OBJECTS_3D)
	$(CC) $(OBJECTS_3D) $(LDFLAGS) -o $@

# Generic rule for .cpp -> .o
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

# Clean rule
clean:
	rm -f *.o $(EXECUTABLES)
