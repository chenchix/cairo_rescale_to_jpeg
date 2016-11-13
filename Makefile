CFLAGS=$(shell pkg-config --cflags cairo libjpeg) -g
LIBS=$(shell pkg-config --libs cairo libjpeg)
TARGET=rescale
SOURCE=jpgtocairo.c


all:
	gcc $(SOURCE) -o $(TARGET) $(CFLAGS) $(LIBS) $(CFLAGS) $(LIBS)  

clean:
	rm -rf $(TARGET)
