CC= g++
#CXXFLAGS= -g -std=c++11
CXXFLAGS= -g -Wno-psabi

SRCS=\
	Alarm.cpp \
	Api.cpp \
	ArchiveManager.cpp \
	ArchivePacket.cpp \
	BitConverter.cpp \
	CommandSocket.cpp \
	CurrentWeather.cpp \
	CurrentWeatherPublisher.cpp \
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
    VantageDecoder.cpp \
	VantageDriver.cpp \
	VantageLogger.cpp \
	VantageWeatherStation.cpp \
	Weather.cpp \
	WeatherSenseSocket.cpp \
	WindDirectionSlice.cpp


OBJDIR=../../target/VP2
OBJLIST=$(SRCS:.cpp=.o)
OBJS=$(addprefix $(OBJDIR)/, $(OBJLIST))

$(OBJDIR)/%.o : %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@
	
all: vp2

vp2: $(OBJDIR) $(OBJS)
	$(CC) -g -o vp2 $(OBJS) -lpthread

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
