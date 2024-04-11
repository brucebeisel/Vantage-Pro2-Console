CC= g++
CXXFLAGS= -g -I../vws -std=c++20 -Wno-psabi

SRCS=\
	DominantWindTest.cpp \
	EnumTest.cpp \
	BitConverterTest.cpp \
	SummaryTest.cpp \
	WindDirectionSliceTest.cpp \
	LoggerTest.cpp \
	ArchiveManagerTest.cpp


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

SUMMARYOBJS= $(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/ArchivePacket.o \
	$(VWSTESTOBJDIR)/ArchiveManager.o \
	$(VWSTESTOBJDIR)/VantageLogger.o \
	$(VWSTESTOBJDIR)/BitConverter.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o \
	$(VWSTESTOBJDIR)/VantageDecoder.o \
	$(VWSTESTOBJDIR)/VantageWeatherStation.o \
	$(VWSTESTOBJDIR)/VantageCRC.o \
	$(VWSTESTOBJDIR)/LoopPacket.o \
	$(VWSTESTOBJDIR)/Loop2Packet.o \
	$(VWSTESTOBJDIR)/HiLowPacket.o \
	$(VWSTESTOBJDIR)/CalibrationAdjustmentsPacket.o \
	$(VWSTESTOBJDIR)/SerialPort.o \
	$(VWSTESTOBJDIR)/SummaryReport.o \
	$(VWSTESTOBJDIR)/UnitConverter.o \
	$(VWSTESTOBJDIR)/WindRoseData.o 
        
	
ARCHIVEOBJS= $(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/ArchivePacket.o \
	$(VWSTESTOBJDIR)/ArchiveManager.o \
	$(VWSTESTOBJDIR)/VantageLogger.o \
	$(VWSTESTOBJDIR)/BitConverter.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o \
	$(VWSTESTOBJDIR)/VantageDecoder.o \
	$(VWSTESTOBJDIR)/VantageWeatherStation.o \
	$(VWSTESTOBJDIR)/UnitConverter.o \
	$(VWSTESTOBJDIR)/VantageCRC.o \
	$(VWSTESTOBJDIR)/LoopPacket.o \
	$(VWSTESTOBJDIR)/Loop2Packet.o \
	$(VWSTESTOBJDIR)/HiLowPacket.o \
	$(VWSTESTOBJDIR)/CalibrationAdjustmentsPacket.o \
	$(VWSTESTOBJDIR)/SerialPort.o
	
LOGGEROBJS= $(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/VantageLogger.o
	
all: DominantWindTest EnumTest BitConverterTest WindDirectionSliceTest SummaryTest ArchiveTest LoggerTest

WindDirectionSliceTest: $(OBJDIR) $(OBJDIR)/WindDirectionSliceTest.o
	$(CC) -g -o WindDirectionSliceTest $(OBJDIR)/WindDirectionSliceTest.o $(DOMWINDOBJS)

DominantWindTest: $(OBJDIR) $(OBJDIR)/DominantWindTest.o
	$(CC) -g -o DominantWindTest $(OBJDIR)/DominantWindTest.o $(DOMWINDOBJS)
	
EnumTest: $(OBJDIR) $(OBJDIR)/EnumTest.o
	$(CC) -g -o EnumTest $(OBJDIR)/EnumTest.o $(ENUMOBJS)
	
BitConverterTest: $(OBJDIR) $(OBJDIR)/BitConverterTest.o
	$(CC) -g -o BitConverterTest $(OBJDIR)/BitConverterTest.o $(BITCONVERTEROBJS)

SummaryTest: $(OBJDIR) $(OBJDIR)/SummaryTest.o
	$(CC) -g -o SummaryTest $(OBJDIR)/SummaryTest.o $(SUMMARYOBJS)

ArchiveTest: $(OBJDIR) $(OBJDIR)/ArchiveManagerTest.o
	$(CC) -g -o ArchiveTest $(OBJDIR)/ArchiveManagerTest.o $(ARCHIVEOBJS)

LoggerTest: $(OBJDIR) $(OBJDIR)/LoggerTest.o
	$(CC) -g -o LoggerTest $(OBJDIR)/LoggerTest.o $(LOGGEROBJS)

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
