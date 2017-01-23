# makefile
CXX=g++ -std=c++11 -Wall

all:
	$(CXX) -g -o approx-pi *.cpp

clean:
	rm -f *.o core.* approx-pi