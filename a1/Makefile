all: addpeer removepeer addcontent removecontent lookupcontent allkeys

addpeer: addpeer.o process.o mybind.o pickip.o peer.o core.o
	g++ addpeer.o process.o mybind.o pickip.o peer.o core.o -o addpeer

removepeer: removepeer.o core.o mybind.o
	g++ removepeer.o core.o mybind.o -o removepeer

addcontent: addcontent.o core.o mybind.o
	g++ addcontent.o core.o mybind.o -o addcontent

removecontent: removecontent.o core.o mybind.o
	g++ removecontent.o core.o mybind.o -o removecontent

lookupcontent: lookupcontent.o core.o mybind.o
	g++ lookupcontent.o core.o mybind.o -o lookupcontent

allkeys: allkeys.o core.o mybind.o
	g++ allkeys.o core.o mybind.o -o allkeys

addpeer.o: addpeer.cpp
	g++ -c addpeer.cpp -o addpeer.o

removepeer.o: removepeer.cpp
	g++ -c removepeer.cpp -o removepeer.o

addcontent.o: addcontent.cpp
	g++ -c addcontent.cpp -o addcontent.o

removecontent.o: removecontent.cpp
	g++ -c removecontent.cpp -o removecontent.o

lookupcontent.o: lookupcontent.cpp
	g++ -c lookupcontent.cpp -o lookupcontent.o

allkeys.o: allkeys.cpp
	g++ -c allkeys.cpp -o allkeys.o

process.o: process.cpp
	g++ -c process.cpp -o process.o

mybind.o: mybind.cpp
	g++ -c mybind.cpp -o mybind.o

pickip.o: pickip.cpp
	g++ -c pickip.cpp -o pickip.o

peer.o: peer.cpp
	g++ -c peer.cpp -o peer.o

core.o: core.cpp
	g++ -c core.cpp -o core.o

clean:
	rm *o addpeer removepeer addcontent removecontent lookupcontent allkeys
