CC=g++ -std=c++11 -g -pthread

dijkstra: dijkstra.o
	${CC} dijkstra.o -o dijkstra

dijkstra.o: dijkstra.cpp
	${CC} -c dijkstra.cpp

.PHONY: clean
clean:
	rm *.o dijkstra
