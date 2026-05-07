#pragma once

namespace NECRO
{
namespace World
{
// Status of the sockets during communication
/*
* 1. Socket is created, and we are waiting for the client to send the first packet
* 2. Client sends the packet [GREETCODE | ENCRYPTED_PACKET]. Using the GREETCODE the World Server can retrieve the sessionKey from the DB  and use it decrypt the packet and start the communication.
*/
enum class WorldSocketStatus
{
    GATHER_INFO,
    GATHER_INFO_PENDING,
    AUTHED,
    CLOSED
};

}
}