all: protocol

protocol: app.o byteStuffing.o main.o
		gcc -o protocol app.o byteStuffing.o main.o
		
app.o: app.c constants.h
		gcc -o app.o -c app.c -w
		
byteStuffing.o: byteStuffing.c
				gcc -o byteStuffing.o -c byteStuffing.c -w
				
main.o: main.c app.h byteStuffing.h
		gcc -o main.o -c main.c -w
		
clean:
	rm -rf *.o
