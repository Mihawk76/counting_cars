all:
	g++ bg.cpp -o bg `pkg-config opencv --cflags --libs`
