CC = gcc
OBJFILES = drv.o fat8.o main.o
TARGET = prog

$(TARGET): $(OBJFILES)
	$(CC) -o $(TARGET) $(OBJFILES)

drv.o: drv.c drv.h
	$(CC) -c drv.c

fat8.o: fat8.c fat8.h
	$(CC) -c fat8.c

main.o: main.c drv.h fat8.h
	$(CC) -c main.c



