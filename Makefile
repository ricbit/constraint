all : hashi.png

hashi : hashi.cc Makefile constraint.h
	g++ -std=c++14 hashi.cc -o hashi -O3 -Wall -g

hashi.dot : hashi hashi.txt
	./hashi < hashi.txt

hashi.png : hashi.dot
	dot -Kfdp hashi.dot -Tpng -o hashi.png
