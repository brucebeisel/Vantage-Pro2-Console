CC= g++
CXXFLAGS= -g -I../vws -std=c++20 -Wno-psabi

SRCS=\
	ArchiveManagerTest.cpp \
	ArchivePacketTest.cpp \
	BitConverterTest.cpp \
	CommandSocketTest.cpp \
	DataCommandHandlerTest.cpp \
	DateTimeFieldsTest.cpp \
	DominantWindTest.cpp \
	EnumTest.cpp \
	LoggerTest.cpp \
	StormArchiveManagerTest.cpp \
	StormDataTest.cpp \
	SummaryTest.cpp \
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

DOMWINDINJOBJS=\
	$(VWSTESTOBJDIR)/DominantWindDirections.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o \
	$(VWSTESTOBJDIR)/WindDirectionSlice.o \
	$(VWSTESTOBJDIR)/LoopPacket.o \
	$(VWSTESTOBJDIR)/Loop2Packet.o \
	$(VWSTESTOBJDIR)/VantageDecoder.o \
	$(VWSTESTOBJDIR)/VantageCRC.o \
	$(VWSTESTOBJDIR)/BitConverter.o \
	$(VWSTESTOBJDIR)/VantageLogger.o \
	$(VWSTESTOBJDIR)/Weather.o 
	
ENUMOBJS=\
	$(VWSTESTOBJDIR)/Weather.o 
	
BITCONVERTEROBJS= \
	$(VWSTESTOBJDIR)/BitConverter.o 

SUMMARYOBJS= \
	$(VWSTESTOBJDIR)/Weather.o \
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
        
	
ARCHIVEMANAGEROBJS= \
	$(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/ArchivePacket.o \
	$(VWSTESTOBJDIR)/ArchiveManager.o \
	$(VWSTESTOBJDIR)/VantageLogger.o \
	$(VWSTESTOBJDIR)/BitConverter.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o \
	$(VWSTESTOBJDIR)/VantageDecoder.o \
	$(VWSTESTOBJDIR)/UnitConverter.o \
	$(VWSTESTOBJDIR)/VantageCRC.o \
	$(VWSTESTOBJDIR)/LoopPacket.o \
	$(VWSTESTOBJDIR)/Loop2Packet.o \
	$(VWSTESTOBJDIR)/HiLowPacket.o \
	$(VWSTESTOBJDIR)/CalibrationAdjustmentsPacket.o \
	$(VWSTESTOBJDIR)/SerialPort.o
	
ARCHIVEPACKETOBJS= \
	$(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/ArchivePacket.o \
	$(VWSTESTOBJDIR)/VantageLogger.o \
	$(VWSTESTOBJDIR)/BitConverter.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o \
	$(VWSTESTOBJDIR)/VantageDecoder.o
	
COMMANDQUEUEOBJS= \
	$(VWSTESTOBJDIR)/CommandQueue.o \
	$(VWSTESTOBJDIR)/CommandData.o \
	$(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/VantageLogger.o

COMMANDSOCKETOBJS= \
	$(VWSTESTOBJDIR)/CommandSocket.o \
	$(VWSTESTOBJDIR)/CommandData.o \
	$(VWSTESTOBJDIR)/CommandHandler.o \
	$(VWSTESTOBJDIR)/CommandQueue.o \
	$(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/VantageLogger.o

DATACOMMANDHANDLEROBJS= \
	$(VWSTESTOBJDIR)/BitConverter.o \
	$(VWSTESTOBJDIR)/DataCommandHandler.o \
	$(VWSTESTOBJDIR)/CommandHandler.o \
	$(VWSTESTOBJDIR)/CommandData.o \
	$(VWSTESTOBJDIR)/DominantWindDirections.o \
	$(VWSTESTOBJDIR)/WindDirectionSlice.o \
	$(VWSTESTOBJDIR)/StormArchiveManager.o \
	$(VWSTESTOBJDIR)/ArchiveManager.o \
	$(VWSTESTOBJDIR)/CurrentWeatherManager.o \
	$(VWSTESTOBJDIR)/CurrentWeather.o \
	$(VWSTESTOBJDIR)/CommandQueue.o \
	$(VWSTESTOBJDIR)/ArchivePacket.o \
	$(VWSTESTOBJDIR)/LoopPacket.o \
	$(VWSTESTOBJDIR)/Loop2Packet.o \
	$(VWSTESTOBJDIR)/StormData.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o \
	$(VWSTESTOBJDIR)/VantageDecoder.o \
	$(VWSTESTOBJDIR)/VantageCRC.o \
	$(VWSTESTOBJDIR)/UnitConverter.o \
	$(VWSTESTOBJDIR)/SummaryReport.o \
	$(VWSTESTOBJDIR)/WindRoseData.o \
	$(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/GraphDataRetriever.o \
	$(VWSTESTOBJDIR)/VantageWeatherStation.o \
	$(VWSTESTOBJDIR)/ForecastRule.o \
	$(VWSTESTOBJDIR)/SerialPort.o \
	$(VWSTESTOBJDIR)/CalibrationAdjustmentsPacket.o \
	$(VWSTESTOBJDIR)/HiLowPacket.o \
	$(VWSTESTOBJDIR)/CurrentWeatherSocket.o \
	$(VWSTESTOBJDIR)/VantageLogger.o

LOGGEROBJS= $(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/VantageLogger.o

DATETIMEFIELDSOBJS= \
	$(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o
	
STORMDATAOBJS= \
	$(VWSTESTOBJDIR)/StormData.o \
	$(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o
	
STORMARCHIVEMANAGEROBJS= \
	$(VWSTESTOBJDIR)/StormArchiveManager.o \
	$(VWSTESTOBJDIR)/StormData.o \
	$(VWSTESTOBJDIR)/SerialPort.o \
	$(VWSTESTOBJDIR)/Weather.o \
	$(VWSTESTOBJDIR)/VantageLogger.o \
	$(VWSTESTOBJDIR)/VantageDecoder.o \
	$(VWSTESTOBJDIR)/BitConverter.o \
	$(VWSTESTOBJDIR)/GraphDataRetriever.o \
	$(VWSTESTOBJDIR)/DateTimeFields.o
	
all: ArchiveManagerTest \
	ArchivePacketTest \
	BitConverterTest \
	CommandSocketTest \
	DateTimeFieldsTest \
	DominantWindTest \
	DominantWindInjectionTest \
	EnumTest \
	LoggerTest \
	StormArchiveManagerTest \
	StormDataTest \
	SummaryTest \
	WindDirectionSliceTest

ArchiveManagerTest: $(ARCHIVEMANAGEROBJS) $(OBJDIR)/ArchiveManagerTest.o
	$(CC) -g -o ArchiveManagerTest $(OBJDIR)/ArchiveManagerTest.o $(ARCHIVEMANAGEROBJS)

ArchivePacketTest: $(ARCHIVEPACKETOBJS) $(OBJDIR)/ArchivePacketTest.o
	$(CC) -g -o ArchivePacketTest $(OBJDIR)/ArchivePacketTest.o $(ARCHIVEPACKETOBJS)

BitConverterTest: $(BITCONVERTEROBJS) $(OBJDIR)/BitConverterTest.o
	$(CC) -g -o BitConverterTest $(OBJDIR)/BitConverterTest.o $(BITCONVERTEROBJS)

CommandQueueTest: $(COMMANDQUEUEOBJS) $(OBJDIR)/CommandQueueTest.o
	$(CC) -g -o CommandQueueTest $(OBJDIR)/CommandQueueTest.o $(COMMANDQUEUEOBJS)

CommandSocketTest: $(COMMANDSOCKETOBJS) $(OBJDIR)/CommandSocketTest.o
	$(CC) -g -o CommandSocketTest $(OBJDIR)/CommandSocketTest.o $(COMMANDSOCKETOBJS) -lpthread

DataCommandHandlerTest: $(DATACOMMANDHANDLEROBJS) $(OBJDIR)/DataCommandHandlerTest.o
	$(CC) -g -o DataCommandHandlerTest $(OBJDIR)/DataCommandHandlerTest.o $(DATACOMMANDHANDLEROBJS)

DateTimeFieldsTest: $(DATETIMEFIELDSOBJS) $(OBJDIR)/DateTimeFieldsTest.o
	$(CC) -g -o DateTimeFieldsTest $(OBJDIR)/DateTimeFieldsTest.o $(DATETIMEFIELDSOBJS)

DominantWindTest: $(DOMWINDOBJS) $(OBJDIR)/DominantWindTest.o
	$(CC) -g -o DominantWindTest $(OBJDIR)/DominantWindTest.o $(DOMWINDOBJS)
	
DominantWindInjectionTest: $(DOMWINDINJOBJS) $(OBJDIR)/DominantWindInjectionTest.o
	$(CC) -g -o DominantWindInjectionTest $(OBJDIR)/DominantWindInjectionTest.o $(DOMWINDINJOBJS)
	
EnumTest: $(ENUMOBJS) $(OBJDIR)/EnumTest.o
	$(CC) -g -o EnumTest $(OBJDIR)/EnumTest.o $(ENUMOBJS)
	
LoggerTest: $(LOGGEROBJS) $(OBJDIR)/LoggerTest.o
	$(CC) -g -o LoggerTest $(OBJDIR)/LoggerTest.o $(LOGGEROBJS)

StormArchiveManagerTest: $(STORMARCHIVEMANAGEROBJS) $(OBJDIR)/StormArchiveManagerTest.o
	$(CC) -g -o StormArchiveManagerTest $(OBJDIR)/StormArchiveManagerTest.o $(STORMARCHIVEMANAGEROBJS)

StormDataTest: $(STORMDATAOBJS) $(OBJDIR)/StormDataTest.o
	$(CC) -g -o StormDataTest $(OBJDIR)/StormDataTest.o $(STORMDATAOBJS)

SummaryTest: $(SUMMARYOBJS) $(OBJDIR)/SummaryTest.o
	$(CC) -g -o SummaryTest $(OBJDIR)/SummaryTest.o $(SUMMARYOBJS)

WindDirectionSliceTest: $(DOMWINDOBJS) $(OBJDIR)/WindDirectionSliceTest.o
	$(CC) -g -o WindDirectionSliceTest $(OBJDIR)/WindDirectionSliceTest.o $(DOMWINDOBJS)

clean:
	rm -f $(OBJS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

depend:
	rm -f Makefile.depend; \
	for i in $(SRCS); do \
		$(CC) -I../vws -MM -MT "$(OBJDIR)/`basename $$i .cpp`.o" $$i >> Makefile.depend; \
	done;

include Makefile.depend
