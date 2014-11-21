#Variables
CC=gcc
LINKER=gcc
FLAGS=


#Build Targets
EbmlWriter.o: EbmlWriter.c EbmlWriter.h
	$(CC) $(FLAGS) -c EbmlWriter.c

EbmlBufferWriter.o: EbmlBufferWriter.c EbmlBufferWriter.h
	$(CC) $(FLAGS) -c EbmlBufferWriter.c
	
MkvElement.o: MkvElement.c WebMElement.h
	$(CC) $(FLAGS) -c MkvElement.c
	
testlibmkv.o: testlibmkv.c
	$(CC) $(FLAGS) -c testlibmkv.c
	
testlibmkv: testlibmkv.o MkvElement.o EbmlBufferWriter.o EbmlWriter.o
	$(LINKER) $(FLAGS) testlibmkv.o MkvElement.o EbmlBufferWriter.o EbmlWriter.o -o testlibmkv

clean:
	rm -rf *.o testlibmkv
	