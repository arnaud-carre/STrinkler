CXX=g++
LD=g++

OBJS=STrinkler.o BinaryBlob.o

TARGET=STrinkler

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
