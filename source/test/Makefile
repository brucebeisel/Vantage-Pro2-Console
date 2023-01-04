CC= g++
#CXXFLAGS= -g -std=c++11
CXXFLAGS= -g -Wno-psabi

SRCS=\
	DominantWindTest.cpp


OBJDIR=../../target/test
VP2TESTOBJDIR=../../target/vp2
OBJLIST=$(SRCS:.cpp=.o)
OBJS=$(addprefix $(OBJDIR)/, $(OBJLIST))

$(OBJDIR)/%.o : %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@
	
DOMWINDOBJS=\
	$(VP2TESTOBJDIR)/DominantWindDirections.o \
	$(VP2TESTOBJDIR)/WindDirectionSlice.o \
	$(VP2TESTOBJDIR)/VP2Logger.o \
	$(VP2TESTOBJDIR)/Weather.o 
	
all: DominantWindTest

DominantWindTest: $(OBJDIR) $(OBJDIR)/DominantWindTest.o
	$(CC) -g -o DominantWindTest $(OBJS) $(DOMWINDOBJS)

clean:
	rm $(OBJS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

depend:
	rm -f Makefile.depend; \
	for i in $(SRCS); do \
		$(CC) -MM -MT "$(OBJDIR)/`basename $$i .cpp`.o" $$i >> Makefile.depend; \
	done;

include Makefile.depend