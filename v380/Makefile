output: v380.o UtlSocket.o UtlDiscovery.o aes.o
	g++ v380.o UtlSocket.o UtlDiscovery.o aes.o -o v380

v380.o: v380.cpp stdafx.h
	g++ -c v380.cpp
	
UtlSocket.o: UtlSocket.cpp UtlSocket.h
	g++ -c UtlSocket.cpp
	
UtlDiscovery.o: UtlDiscovery.cpp UtlDiscovery.h
	g++ -c UtlDiscovery.cpp
	
aes.o: aes.cpp aes.h
	g++ -c aes.cpp

clean:
	rm *.o v380