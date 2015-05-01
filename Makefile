all : hashi.png

hashi : hashi.cc
	g++ -std=c++14 hashi.cc -o hashi -O3 -Wall

hashi.dot : hashi hashi.txt
	./hashi < hashi.txt

hashi.png : hashi.dot
	dot -Kfdp hashi.dot -Tpng -o hashi.png
