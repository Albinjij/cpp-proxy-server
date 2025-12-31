CXX = g++
CXXFLAGS = -Wall -pthread -std=c++11
SRC = src/main.cpp src/proxy_logic.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = proxy_server

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

