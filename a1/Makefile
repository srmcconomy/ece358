COMMONREQ = core.cpp mybind.c

all: addpeer removepeer addcontent removecontent lookupcontent allkeys

addpeer: addpeer.cpp process.cpp peer.cpp pickip.c $(COMMONREQ)
	g++ $@.cpp -o $@

removepeer: removepeer.cpp $(COMMONREQ)
	g++ $@.cpp -o $@

addcontent: addcontent.cpp $(COMMONREQ)
	g++ $@.cpp -o $@

removecontent: removecontent.cpp $(COMMONREQ)
	g++ $@.cpp -o $@

lookupcontent: lookupcontent.cpp $(COMMONREQ)
	g++ $@.cpp -o $@

allkeys: allkeys.cpp $(COMMONREQ)
	g++ $@.cpp -o $@

clean:
	rm addpeer removepeer addcontent removecontent lookupcontent allkeys
