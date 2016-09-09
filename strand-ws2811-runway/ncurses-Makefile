CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g -Wall -Werror -std=c++11
LDFLAGS=-g
LDLIBS=-lncurses

SRCS=digital-strand.cc
OBJS=$(subst .cc,.o,$(SRCS))

all: digital-strand

digital-strand: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) *~ .depend

include .depend
