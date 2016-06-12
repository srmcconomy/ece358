all: addpeer removepeer addcontent removecontent lookupcontent allkeys

addpeer: addpeer.o process.o mybind.o pickip.o peer.o core.o
	g++ $^ -o addpeer

removepeer: removepeer.o core.o mybind.o
	g++ $^ -o removepeer

addcontent: addcontent.o core.o mybind.o
	g++ $^ -o addcontent

removecontent: removecontent.o core.o mybind.o
	g++ $^ -o removecontent

lookupcontent: lookupcontent.o core.o mybind.o
	g++ $^ -o lookupcontent

allkeys: core.o mybind.o allkeys.o
	g++ $^ -o allkeys

addpeer.o: addpeer.cpp
	g++ -c $^ -o $@

removepeer.o: removepeer.cpp
	g++ -c $^ -o $@

addcontent.o: addcontent.cpp
	g++ -c $^ -o $@

removecontent.o: removecontent.cpp
	g++ -c $^ -o $@

lookupcontent.o: lookupcontent.cpp
	g++ -c $^ -o $@

allkeys.o: allkeys.cpp
	g++ -c $^ -o $@

process.o: process.cpp
	g++ -c $^ -o $@

mybind.o: mybind.c
	g++ -c $^ -o $@

pickip.o: pickip.c
	g++ -c $^ -o $@

peer.o: peer.cpp
	g++ -c $^ -o $@

core.o: core.cpp
	g++ -c $^ -o $@

clean:
	rm *o addpeer removepeer addcontent removecontent lookupcontent allkeys
