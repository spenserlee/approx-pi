# makefile
CXX=g++ -Wall

all:
	$(CXX) -g -o a1 *.cpp

clean:
	rm -f *.o core.* a1