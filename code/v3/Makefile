target: gradingServer

gradingServer: main.o thread_func.o master_func.o start_function.o queue.o 
	cc -o gradingServer *.o
	
main.o: main.c CustomFunction.h variables.h

thread_func.o: thread_func.c CustomFunction.h variables.h

master_func.o: master_func.c CustomFunction.h variables.h
 
start_function.o: start_function.c CustomFunction.h variables.h
 
queue.o: queue.c CustomFunction.h variables.h

clean:
	rm -f *.o gradingServer
