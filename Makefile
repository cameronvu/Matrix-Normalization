# CMSC216 Project 5 Makefile
AN = p5
CLASS = 216

# -Wno-unused : don't warn for unused vars
# Provide debug level optimizations
CFLAGS = -Wall -Wno-comment -Werror -g -Og -msse3 -Wno-unused-result
CC     = gcc $(CFLAGS)
SHELL  = /bin/bash
CWD    = $(shell pwd | sed 's/.*\///g')

export PARALLEL?=True		#enable parallel testing if not overridden
export LANG=C			#sets locale to avoid unicode quotes

PROGRAMS = \
	el_malloc.o \
	el_demo \
	test_el_malloc \
	colnorm_print \
	colnorm_benchmark \

############################################################
# Default target and cleaning target to remove compiled programs/objects
all : $(PROGRAMS)

clean :
	rm -f $(PROGRAMS) *.o vgcore.* 

############################################################
# help message to show build targets
help :
	@echo 'Typical usage is:'
	@echo '  > make                          # build all programs'
	@echo '  > make clean                    # remove all compiled items'
	@echo '  > make zip                      # create a zip file for submission'
	@echo '  > make prob1                    # built targets associated with problem 1'
	@echo '  > make prob1 testnum=5          # run problem 1 test #5 only'
	@echo '  > make test-prob2               # run test for problem 2'
	@echo '  > make test                     # run all tests'
	@echo '  > make update                   # download and install any updates to project files'

############################################################
# 'make zip' to create complete.zip for submission
ZIPNAME = $(AN)-complete.zip
zip : clean clean-tests
	rm -f $(ZIPNAME)
	cd .. && zip "$(CWD)/$(ZIPNAME)" -r "$(CWD)"
	@echo Zip created in $(ZIPNAME)
	@if (( $$(stat -c '%s' $(ZIPNAME)) > 10*(2**20) )); then echo "WARNING: $(ZIPNAME) seems REALLY big, check there are no abnormally large test files"; du -h $(ZIPNAME); fi
	@if (( $$(unzip -t $(ZIPNAME) | wc -l) > 256 )); then echo "WARNING: $(ZIPNAME) has 256 or more files in it which may cause submission problems"; fi

############################################################
# `make update` to get project updates
update :
	curl https://www.cs.umd.edu/~profk/216/$(AN)-update.sh | /bin/bash 

################################################################################
# build .o files from corresponding .c files
%.o : %.c
	$(CC) -c $<

################################################################################
# EL MALLOC
el_malloc.o : el_malloc.c el_malloc.h
	$(CC) -c $<

el_demo : el_demo.c el_malloc.o
	$(CC) -o $@ $^

test_el_malloc : test_el_malloc.c el_malloc.o
	$(CC) -o $@ $^

################################################################################
# Matrix column normalization optimization problem
colnorm_print : colnorm_print.o colnorm_util.o colnorm_base.o colnorm_optm.o
	$(CC) -o $@ $^ -lm -lpthread

colnorm_benchmark : colnorm_benchmark.o colnorm_util.o colnorm_base.o colnorm_optm.o
	$(CC) -o $@ $^ -lm -lpthread


################################################################################
# Testing Targets
test: test-prob1 test-prob2

test-setup :
	@chmod u+rx testy

test-prob1: el_demo test_el_malloc test-setup el_demo
	./testy -o md test_el_malloc.org $(testnum)

test-prob2: colnorm_benchmark colnorm_print test-setup
	./testy -o md test_colnorm.org $(testnum)

clean-tests :
	rm -rf test-results


