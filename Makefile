CXX := g++
CXXFLAGS := -std=c++17 -O2
LDFLAGS := -lglfw -lGLEW -lGL -ldl -lpthread

SRC := main.cpp \
       GLTFRenderer.cpp \
       GLTFLoader.cpp \
       Textures.cpp \
       Physics.cpp \
       Camera.cpp \
       Render.cpp

BIN := gltf_renderer

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: all run clean
