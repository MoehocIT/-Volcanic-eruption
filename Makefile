CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++98
TARGET = volcano_demo
INTEGRATED_TARGET = volcano_scene
CORE_OBJECTS = src/VolcanoScene.o src/VolcanoEffects.o
AR = ar

all: libvolcano.a

libvolcano.a: $(CORE_OBJECTS)
	$(AR) rcs $@ $(CORE_OBJECTS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

demo: $(TARGET)

integrated-demo: $(INTEGRATED_TARGET)

$(TARGET): src/demo_main.cpp $(CORE_OBJECTS)
	$(CXX) $(CXXFLAGS) src/demo_main.cpp $(CORE_OBJECTS) -o $@ -lglut -lGLU -lGL

$(INTEGRATED_TARGET): src/main.cpp $(CORE_OBJECTS)
	$(CXX) $(CXXFLAGS) src/main.cpp $(CORE_OBJECTS) -o $@ -lglut -lGLU -lGL

clean:
	rm -f $(TARGET) $(INTEGRATED_TARGET) libvolcano.a $(CORE_OBJECTS)

.PHONY: all demo integrated-demo clean
