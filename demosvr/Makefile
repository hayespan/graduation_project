include ../Makefile.comm

COMM_OBJS = demosvrmeta.o demosvrclass.pb.o
CLI_OBJS = demosvrclient.o demosvrcliproto.o $(COMM_OBJS)
SVR_OBJS = demosvrmain.o demosvrdispatcher.o demosvrservice.o $(COMM_OBJS)
TEST_OBJS = testclient.o

all: CXXFLAGS += -I$(ROOT_DIR) 
all: demosvrclient demosvr testclient
all: INCLS += -I$(ROOT_DIR)
all: DEPS += -lprotobuf -L$(DYNAMIC_DIR) -lhaysvr -lhaycomm


.PHONY: demosvrclient
demosvrclient: libdemosvrclient.so libdemosvrclient.a

libdemosvrclient.so: $(CLI_OBJS)
	$(CXX_DYNAMIC)
libdemosvrclient.a: $(CLI_OBJS)
	$(AR_STATIC)

demosvr: CXXFLAGS += -static
demosvr: INCLS = 
demosvr: DEPS = -lprotobuf -L$(STATIC_DIR) -lhaysvr -lhaycomm -lpthread
demosvr: $(SVR_OBJS) 
	$(CXX_EXE)

testclient: DEPS += -L$(DYNAMIC_DIR) -ldemosvrclient -lhaysvrcli -lhaycomm
testclient: $(TEST_OBJS)
	$(CXX_EXE)

.PHONY: clean
clean:
	rm -f *.o;
	rm -f *.so;
	rm -f *.a;
	rm -f demosvr;
	rm -f testclient;
