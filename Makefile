
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++98

TARGET = volcano_demo.exe

SOURCES = src/main.cpp src/VolcanoScene.cpp src/VolcanoEffects.cpp

LIBS = -lfreeglut -lglu32 -lopengl32

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@ $(LIBS)

clean:
	rm -f $(TARGET)
