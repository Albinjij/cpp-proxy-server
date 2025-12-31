CC = g++
CFLAGS = -std=c++11 -pthread
TARGET = proxy
SRC = src/proxy.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
