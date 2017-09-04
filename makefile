all:
	g++ -std=c++0x bg.cpp -o bg `pkg-config opencv --cflags --libs`
