CC = g++
OPENCV = `pkg-config --cflags --libs opencv`

prog = edgedetector

default:	 $(prog)

$(prog): 	$(prog).cpp
				$(CC) $(prog).cpp -o $(prog) $(OPENCV)

gdb:	 	 $(prog)DB
				gdb openvid

$(prog)DB: $(prog).cpp
				$(CC) -g $(prog).cpp -o $(prog)DB $(OPENCV)

valgrind: 	$(prog)
				valgrind ./$(prog) -i ../../f1.mp4 -o ../../f1.avi

formart: 
				astyle -A9 $(prog).cpp

clean : 
				$(RM) $(prog) *.o *.orig
