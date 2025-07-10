#include "Packet.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <sstream>
#include <iomanip>

//--------------------------------------------------------------------------------------------
// Appends bytes to the data
//--------------------------------------------------------------------------------------------
void Packet::Append(uint8_t const* src, size_t cnt)
{
	// Calculate the new size of this packet
	size_t const newSize = wpos + cnt;

	// Check if the capacity (reserved bytes) isn't enough 
	if (data.capacity() < newSize) 
	{
		data.reserve(newSize + (newSize / 2));	// Increase the capacity to the needed capacity + an extra 50%
	}

	// Resize the vector if the space is not enough
	if(data.size() < newSize)
		data.resize(newSize);

	std::memcpy(&data[wpos], src, cnt);
	wpos = newSize;
}

//--------------------------------------------------------------------------------------------
// Prints the packet in hex
//--------------------------------------------------------------------------------------------
void Packet::Print() const
{
	LOG_INFO("Packet size %d", Size());
	
	// Hex print of the packet content
	std::ostringstream oss;  // Output string stream

	for (size_t i = 0; i < data.size(); ++i) 
	{
		// Print each byte in hexadecimal format into the string stream
		oss << std::hex << std::setfill('0') << std::setw(2)
			<< static_cast<int>(data[i]) << " ";

		// Group in 16 bytes
		if ((i + 1) % 16 == 0) {
			oss << std::endl;
		}
	}

	oss << std::endl;

	LOG_INFO("Packet content: \n%s", oss.str().c_str());
}
