COMPILE 	= g++ -c -std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow
LINK 		= g++ -o

httpserver: httpserver.o
	${LINK} $@ $^

httpserver.o: httpserver.cpp
	${COMPILE} httpserver.cpp

clean:
	rm httpserver *.o