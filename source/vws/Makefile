CC= g++
CXXFLAGS= -g -std=c++20 -Wno-psabi
PROGRAM= vws

SRCS=\
	Alarm.cpp \
	AlarmManager.cpp \
	AlarmProperties.cpp \
	ArchiveManager.cpp \
	ArchivePacket.cpp \
	BaudRate.cpp \
	BitConverter.cpp \
    CalibrationAdjustmentsPacket.cpp \
	CommandData.cpp \
	CommandHandler.cpp \
	ConsoleCommandHandler.cpp \
	ConsoleDiagnosticReport.cpp \
	CommandQueue.cpp \
	CommandSocket.cpp \
	CurrentWeather.cpp \
	CurrentWeatherManager.cpp \
	CurrentWeatherSocket.cpp \
	DataCommandHandler.cpp \
	DateTimeFields.cpp \
	DominantWindDirections.cpp \
	ForecastRule.cpp \
	GraphDataRetriever.cpp \
	HiLowPacket.cpp \
	Loop2Packet.cpp \
	LoopPacket.cpp \
	main.cpp \
 	SerialPort.cpp \
 	StormArchiveManager.cpp \
 	StormData.cpp \
 	SummaryReport.cpp \
	UnitConverter.cpp \
 	UnitsSettings.cpp \
	VantageCRC.cpp \
	VantageConfiguration.cpp \
	VantageDecoder.cpp \
	VantageDriver.cpp \
	VantageLogger.cpp \
	VantageStationNetwork.cpp \
	VantageWeatherStation.cpp \
	Weather.cpp \
	WindDirectionSlice.cpp \
	WindRoseData.cpp


OBJDIR=../../target/vws
INCDIRS=-I../3rdParty
OBJLIST=$(SRCS:.cpp=.o)
OBJS=$(addprefix $(OBJDIR)/, $(OBJLIST))

$(OBJDIR)/%.o : %.cpp
	$(CC) $(CXXFLAGS) $(INCDIRS) -c $< -o $@
	
all: $(PROGRAM)

$(PROGRAM): $(OBJDIR) $(OBJS)
	$(CC) -g -o $(PROGRAM) $(OBJS) -lpthread

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
