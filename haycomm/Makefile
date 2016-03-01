include ../Makefile.comm

OBJECTS = daemonize.o time_util.o log.o lockguard.o config.o 

DYNAMIC_LIBS = libhaycomm.so
STATIC_LIBS = libhaycomm.a
TESTS = test

all: $(DYNAMIC_LIBS) $(STATIC_LIBS) $(TESTS)

libhaycomm.so: $(OBJECTS)
	$(CXX_DYNAMIC)

libhaycomm.a: $(OBJECTS)
	$(AR_STATIC)

test: DEPS = -L$(DYNAMIC_DIR) -lhaycomm 
test: test.o
	$(CXX_EXE)

.PHONY: clean
clean:
	rm -f *.o;
	rm -f $(DYNAMIC_LIBS)
	rm -f $(STATIC_LIBS)
	rm -f $(TESTS)
