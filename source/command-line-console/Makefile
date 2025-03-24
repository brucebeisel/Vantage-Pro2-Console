CC= g++
CXXFLAGS= -g -std=c++20 -Wno-psabi
PROGRAM= command-line-console

SRCS= commandLineConsole.cpp

VWSOBJDIR=../../target/vws
OBJDIR=../../target/test
INCDIRS=-I../vws -I../3rdParty
OBJLIST=$(SRCS:.cpp=.o)
OBJS=$(addprefix $(OBJDIR)/, $(OBJLIST))

VWSOBJS = \
	$(VWSOBJDIR)/Alarm.o \
	$(VWSOBJDIR)/AlarmManager.o \
	$(VWSOBJDIR)/AlarmProperties.o \
	$(VWSOBJDIR)/ArchiveManager.o \
	$(VWSOBJDIR)/ArchivePacket.o \
	$(VWSOBJDIR)/BaudRate.o \
	$(VWSOBJDIR)/BitConverter.o \
	$(VWSOBJDIR)/CalibrationAdjustmentsPacket.o \
	$(VWSOBJDIR)/CommandData.o \
	$(VWSOBJDIR)/CommandHandler.o \
	$(VWSOBJDIR)/CommandQueue.o \
	$(VWSOBJDIR)/ConsoleCommandHandler.o \
	$(VWSOBJDIR)/ConsoleDiagnosticReport.o \
	$(VWSOBJDIR)/DateTimeFields.o \
	$(VWSOBJDIR)/HiLowPacket.o \
	$(VWSOBJDIR)/LoopPacket.o \
	$(VWSOBJDIR)/Loop2Packet.o \
	$(VWSOBJDIR)/SerialPort.o \
	$(VWSOBJDIR)/UnitsSettings.o \
	$(VWSOBJDIR)/VantageCRC.o \
	$(VWSOBJDIR)/VantageDecoder.o \
	$(VWSOBJDIR)/VantageLogger.o \
	$(VWSOBJDIR)/VantageConfiguration.o \
	$(VWSOBJDIR)/VantageStationNetwork.o \
	$(VWSOBJDIR)/VantageWeatherStation.o \
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