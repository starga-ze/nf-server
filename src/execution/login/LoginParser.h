#include "execution/Event.h"
#include "net/packet/ParsedPacket.h"

#include <memory>

class LoginParser
{
public:
    LoginParser() = default;
    ~LoginParser() = default;

    static std::unique_ptr<Event> deserialize(ParsedPacket& parsed);

private:
    static std::unique_ptr<Event> parseLoginReq(ParsedPacket& parsed);
};
