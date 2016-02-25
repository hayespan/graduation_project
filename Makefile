include Makefile.comm

DIRECTORY = haycomm haysvr

all: 
	for dir in $(DIRECTORY); do \
		cd $$(dir); make; \
	done

.PHONY: clean
clean:
	for dir in $(DIRECTORY); do \
		cd $$(dir); make clean; \
	done
