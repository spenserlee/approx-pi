# makefile
CXX=g++ -Wall

all:
	$(CXX) -o a1 *.cpp

clean:
	rm -f *.o core.* a1