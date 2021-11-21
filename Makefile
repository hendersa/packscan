CXXFLAGS=-std=c++11 -Wall -Werror -pedantic -I. -g
OBJS=pack.o shiftjis_conv.o main.o
BIN=packscan

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(BIN): $(OBJS)
	$(CXX) $(OBJS) -o $(BIN)

.PHONY: clean
	
clean:
	rm -f *.o $(BIN)

