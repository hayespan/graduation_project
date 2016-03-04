include ../Makefile.comm

COMM_OBJS = haybuf.o tcpchannel.o
CLI_OBJS = haysvrclient.o haysvrcliproto.o $(COMM_OBJS)
SVR_OBJS = haysvr.o haysvrdispatcher.o tcpsvr.o $(COMM_OBJS)

all: cli svr
all: CXXFLAGS += -I$(ROOT_DIR)
all: INCLS += -I$(ROOT_DIR)
all: DEPS += -L$(DYNAMIC_DIR) -lhaycomm

cli: libhaysvrcli.so libhaysvrcli.a

libhaysvrcli.so: $(CLI_OBJS)
	$(CXX_DYNAMIC)
libhaysvrcli.a: $(CLI_OBJS)
	$(AR_STATIC)

svr: libhaysvr.so libhaysvr.a 
libhaysvr.so: $(SVR_OBJS)
	$(CXX_DYNAMIC)
libhaysvr.a: $(SVR_OBJS)
	$(AR_STATIC)

.PHONY: clean
clean:
	rm -f *.o;
	rm -f *.so
	rm -f *.a;
