CC= g++
CXXFLAGS= -g -std=c++20 -Wno-psabi
PROGRAM= loop-dumper

SRCS= loopDumper.cpp

VWSOBJDIR=../../target/vws
OBJDIR=../../target/test
INCDIRS=-I../vws
OBJLIST=$(SRCS:.cpp=.o)
OBJS=$(addprefix $(OBJDIR)/, $(OBJLIST))

VWSOBJS = \
	$(VWSOBJDIR)/BitConverter.o \
	$(VWSOBJDIR)/CurrentWeather.o \
	$(VWSOBJDIR)/DateTimeFields.o \
	$(VWSOBJDIR)/ForecastRule.o \
	$(VWSOBJDIR)/LoopPacket.o \
	$(VWSOBJDIR)/Loop2Packet.o \
	$(VWSOBJDIR)/VantageCRC.o \
	$(VWSOBJDIR)/VantageDecoder.o \
	$(VWSOBJDIR)/VantageLogger.o \
	$(VWSOBJDIR)/Weather.o


$(OBJDIR)/%.o : %.cpp
	$(CC) $(CXXFLAGS) $(INCDIRS) -c $< -o $@
	
all: $(PROGRAM)

$(PROGRAM): $(OBJDIR) $(OBJS)
	$(CC) -g -o $(PROGRAM) $(OBJS) $(VWSOBJS) -lpthread

clean:
	rm $(OBJS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

depend:
	rm -f Makefile.depend; \
	for i in $(SRCS); do \
		$(CC) $(INCDIRS) -MM -MT "$(OBJDIR)/`basename $$i .cpp`.o" $$i >> Makefile.depend; \
	done;

include Makefile.depend
