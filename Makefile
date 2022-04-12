CXX = g++
CFLAGS = -Wall

logger: hw2.cpp logger.cpp
	$(CXX) $(CFLAGS) -o logger.so -shared -fPIC logger.cpp -ldl 
	$(CXX) $(CFLAGS) -o $@ hw2.cpp

clean:
	rm -f logger logger.so
