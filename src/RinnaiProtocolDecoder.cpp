#include "RinnaiProtocolDecoder.hpp"

// temperatures allowed to set with this code (linear range)
const byte TEMP_C_MIN = 37;
const byte TEMP_C_MAX = 48;

const byte TEMP_MAX_CODE = 0xe; // the max valid code
const byte TEMP_CODE[] = {37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 50, 55, 60};

RinnaiPacketSource RinnaiProtocolDecoder::getPacketSource(const byte *data, int length)
{
	// check packet size
	if (length != BYTES_IN_PACKET)
	{
		return INVALID;
	}
	// check that packet is checksum valid
	byte checksum = 0;
	for (int i = 0; i < BYTES_IN_PACKET; i++)
	{
		checksum ^= data[i];
	}
	if (checksum != 0)
	{
		return INVALID;
	}
	// see who the sender is
	if ((data[0] & 0xf) == 0x7 && data[4] == 0x20)
	{
		return HEATER;
	}
	if ((data[0] & 0xf) < 0x7 && data[4] == 0xbf)
	{
		return CONTROL;
	}
	return UNKNOWN;
}

// assume packet passed getPacketSource==HEATER
bool RinnaiProtocolDecoder::decodeHeaterPacket(const byte *data, RinnaiHeaterPacket &packet)
{
	packet.activeId = (data[0] >> 4) & 0xf;
	packet.inUse = data[2] & 0x10;
	packet.on = data[1] & 0xc0;
	packet.startupState = data[3];
	bool ret = temperatureCodeToTemperatureCelsius(data[2] & 0xf, packet.temperatureCelsius);
	return ret;
}

// assume packet passed getPacketSource==CONTROL
bool RinnaiProtocolDecoder::decodeControlPacket(const byte *data, RinnaiControlPacket &packet)
{
	packet.myId = data[0] & 0xf;
	packet.onOffPressed = data[1] & 0x1;
	packet.priorityPressed = data[1] & 0x4;
	packet.temperatureUpPressed = data[2] & 0x1;
	packet.temperatureDownPressed = data[2] & 0x2;
	return true;
}

bool RinnaiProtocolDecoder::temperatureCodeToTemperatureCelsius(byte code, byte &temperature)
{
	if (code > TEMP_MAX_CODE)
	{
		temperature = 0;
		return false;
	}
	temperature = TEMP_CODE[code];
	return true;
}

void RinnaiProtocolDecoder::calcAndSetChecksum(byte *data)
{
	byte checksum = 0;
	for (int i = 0; i < BYTES_IN_PACKET - 1; i++)
	{
		checksum ^= data[i];
	}
	data[BYTES_IN_PACKET - 1] = checksum;
}

void RinnaiProtocolDecoder::setOnOffPressed(byte *data)
{
	data[1] = (data[1] & (~0x80)) | 0x01; // remove msb and set button bit
	calcAndSetChecksum(data);
}
