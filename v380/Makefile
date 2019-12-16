output: v380.o UtlSocket.o UtlDiscovery.o aes.o
	/usr/local/bin/g++-9 v380.o UtlSocket.o UtlDiscovery.o aes.o -o v380

v380.o: v380.cpp stdafx.h
	/usr/local/bin/g++-9 -c v380.cpp
	
UtlSocket.o: UtlSocket.cpp UtlSocket.h
	/usr/local/bin/g++-9 -c UtlSocket.cpp
	
UtlDiscovery.o: UtlDiscovery.cpp UtlDiscovery.h
	/usr/local/bin/g++-9 -c UtlDiscovery.cpp
	
aes.o: aes.cpp aes.h
	/usr/local/bin/g++-9 -c aes.cpp

clean:
	rm *.o v380
