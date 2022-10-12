# Description: Makefile for building a cbp submission.

CFLAGS = -g -o3 -Wall
CXXFLAGS = -g -o3 -Wall

objects = tracer.o predictor.o main.o 

predictor : $(objects)
	$(CXX) -o $@ $(objects)



clean :
	rm -f predictor $(objects)

