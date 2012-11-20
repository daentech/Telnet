Telnet Server
=============

A simple telnet server for Cisco

Compilation
===========

To compile the server, run
    make

Usage
=====

run 
    telnet_srv

You can provide either of the following arguments:

*-test  Runs the server in test mode. This runs the unit tests before starting
*<portno>   Supplying a port number will run the server on this port

The server by default will use port 6770
