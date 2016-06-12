COMMONREQ = core.cpp mybind.c

all: addpeer removepeer addcontent removecontent lookupcontent allkeys

addpeer: addpeer.cpp process.cpp peer.cpp pickip.c $(COMMONREQ)
	g++ $^ -o $@

removepeer: removepeer.cpp $(COMMONREQ)
	g++ $^ -o $@

addcontent: addcontent.cpp $(COMMONREQ)
	g++ $^ -o $@

removecontent: removecontent.cpp $(COMMONREQ)
	g++ $^ -o $@

lookupcontent: lookupcontent.cpp $(COMMONREQ)
	g++ $^ -o $@

allkeys: allkeys.cpp $(COMMONREQ)
	g++ $^ -o $@

clean:
	rm addpeer removepeer addcontent removecontent lookupcontent allkeys
