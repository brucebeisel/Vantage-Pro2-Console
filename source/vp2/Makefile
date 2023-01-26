CC= g++
CXXFLAGS= -g -std=c++20 -Wno-psabi
PROGRAM= vws

SRCS=\
	Alarm.cpp \
	Api.cpp \
	ArchiveManager.cpp \
	ArchivePacket.cpp \
	BitConverter.cpp \
	CommandHandler.cpp \
	CommandSocket.cpp \
	CurrentWeather.cpp \
	CurrentWeatherManager.cpp \
	CurrentWeatherSocket.cpp \
	DominantWindDirections.cpp \
	EventManager.cpp \
	ForecastRule.cpp \
    HiLowPacket.cpp \
	Loop2Packet.cpp \
	LoopPacket.cpp \
	main.cpp \
	ParametersMessage.cpp \
	ProtocolException.cpp \
	Sensor.cpp \
	SensorStation.cpp \
    SerialPort.cpp \
	UnitConverter.cpp \
	VantageConstants.cpp \
	VantageCRC.cpp \
    VantageConfiguration.cpp \
    VantageDecoder.cpp \
	VantageDriver.cpp \
	VantageLogger.cpp \
	VantageStationNetwork.cpp \
	VantageWeatherStation.cpp \
	Weather.cpp \
	WeatherSenseSocket.cpp \
	WindDirectionSlice.cpp


OBJDIR=../../target/VP2
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
