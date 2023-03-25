CC= g++
CXXFLAGS= -g -I../vws -std=c++20 -Wno-psabi

SRCS=\
	DominantWindTest.cpp \
	EnumTest.cpp \
	BitConverterTest.cpp \
	WindDirectionSliceTest.cpp


OBJDIR=../../target/test
VWSTESTOBJDIR=../../target/vws
OBJLIST=$(SRCS:.cpp=.o)
OBJS=$(addprefix $(OBJDIR)/, $(OBJLIST))

$(OBJDIR)/%.o : %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@
	
DOMWINDOBJS=\
	$(VWSTESTOBJDIR)/DominantWindDirections.o \
	$(VWSTESTOBJDIR)/WindDirectionSlice.o \
	$(VWSTESTOBJDIR)/VantageLogger.o \
	$(VWSTESTOBJDIR)/Weather.o 
	
ENUMOBJS=\
	$(VWSTESTOBJDIR)/Weather.o 
	
BITCONVERTEROBJS= $(VWSTESTOBJDIR)/BitConverter.o 
	
all: DominantWindTest EnumTest BitConverterTest WindDirectionSliceTest

WindDirectionSliceTest: $(OBJDIR) $(OBJDIR)/WindDirectionSliceTest.o
	$(CC) -g -o WindDirectionSliceTest $(OBJDIR)/WindDirectionSliceTest.o $(DOMWINDOBJS)

DominantWindTest: $(OBJDIR) $(OBJDIR)/DominantWindTest.o
	$(CC) -g -o DominantWindTest $(OBJDIR)/DominantWindTest.o $(DOMWINDOBJS)
	
EnumTest: $(OBJDIR) $(OBJDIR)/EnumTest.o
	$(CC) -g -o EnumTest $(OBJDIR)/EnumTest.o $(ENUMOBJS)
	
BitConverterTest: $(OBJDIR) $(OBJDIR)/BitConverterTest.o
	$(CC) -g -o BitConverterTest $(OBJDIR)/BitConverterTest.o $(BITCONVERTEROBJS)

clean:
	rm -f $(OBJS) $(OBJDIR)/EnumTest.o 

$(OBJDIR):
	mkdir -p $(OBJDIR)

depend:
	rm -f Makefile.depend; \
	for i in $(SRCS); do \
		$(CC) -I../vws -MM -MT "$(OBJDIR)/`basename $$i .cpp`.o" $$i >> Makefile.depend; \
	done;

include Makefile.depend
