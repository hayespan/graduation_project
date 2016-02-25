include ../Makefile.comm

OBJECTS = daemonize.o

DYNAMIC_LIBS = libhaycomm.so
STATIC_LIBS = libhaycomm.a
TESTS = test_daemonize

all: $(DYNAMIC_LIBS) $(STATIC_LIBS) $(TESTS)

libhaycomm.so: $(OBJECTS)
	$(CXX_DYNAMIC)

libhaycomm.a: $(OBJECTS)
	$(AR_STATIC)

test_daemonize: DEPS = -L$(DYNAMIC_DIR) -lhaycomm
test_daemonize: test_daemonize.o
	$(CXX_EXE)

.PHONY: clean
clean:
	rm -f *.o;
	rm -f $(DYNAMIC_LIBS)
	rm -f $(STATIC_LIBS)
	rm -f $(TESTS)
