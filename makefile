all:
	g++ unit_test.cpp telnet.cpp -lpthread -o telnet_srv

clean:
	rm -rf *o telnet_srv
