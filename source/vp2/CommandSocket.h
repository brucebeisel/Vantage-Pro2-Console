#ifndef COMMAND_SOCKET_H_
#define COMMAND_SOCKET_H_

namespace vws {

class EventManager;

class CommandSocket {
public:
    CommandSocket(EventManager & evtMgr);
    virtual ~CommandSocket();

private:
    int            listenFd;
    EventManager & eventManager;
};

} /* namespace vws */

#endif /* COMMAND_SOCKET_H_ */
