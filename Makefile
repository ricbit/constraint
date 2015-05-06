all : hashi.png

slither : slither.cc Makefile constraint.h
	g++ -std=c++14 slither.cc -o slither -O3 -Wall -g

fuji : slither
	(for i in `seq 1 34`; do echo "problem $$i";  timeout 1200 ./slither < data/slither.fuji.$$i.txt; done;) > result.txt

hashi : hashi.cc Makefile constraint.h
	g++ -std=c++14 hashi.cc -o hashi -O3 -Wall -g

hashi.dot : hashi data/hashi.txt
	./hashi < data/hashi.txt

hashi.png : hashi.dot
	dot -Kfdp hashi.dot -Tpng -o hashi.png
