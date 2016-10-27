/*	Copyright (c) 2003-2016 Xsens Technologies B.V. or subsidiaries worldwide.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions are met:

	1.	Redistributions of source code must retain the above copyright notice,
		this list of conditions and the following disclaimer.

	2.	Redistributions in binary form must reproduce the above copyright notice,
		this list of conditions and the following disclaimer in the documentation
		and/or other materials provided with the distribution.

	3.	Neither the names of the copyright holders nor the names of their contributors
		may be used to endorse or promote products derived from this software without
		specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
	THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
	OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR
	TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef PACKET_XS_MTIXDISPLAYER_H
#define PACKET_XS_MTIXDISPLAYER_H

#include <xsensdeviceapi.h>
#include "xsmtdisplayer.h"

namespace XsDemo
{

//--------------------------------------------------------------------------------
class MTixDisplayer : public MTDisplayer
{
public:
	MTixDisplayer(XsPortInfo const & portInfo, XsDevice& device)
		: MTDisplayer(portInfo, device)
	{
		MTixDisplayer::setOrientationEuler();
	}

	virtual int displayPacket(int x, int y, std::ostream& out);
	virtual void displayKeyHelpLine(int x, int y, std::ostream& out) const;
	virtual bool handleKeyPress(int key);

protected:
	virtual void setConfig(XsDeviceMode const & mode);

	virtual void setOrientationEuler();
	virtual void setOrientationQuaternion();
	virtual void setOrientationMatrix();
	virtual void setRawData();
};

//--------------------------------------------------------------------------------
void MTixDisplayer::displayKeyHelpLine(int x, int y, std::ostream& out) const
{
	gotoXY(x, y);
	out << "Press 'a' for euler, 's' for quaternion, 'd' for matrix, 'g' for raw";
}

//--------------------------------------------------------------------------------
int MTixDisplayer::displayPacket(int x, int y, std::ostream& out)
{
	int numberOfLines = 0;
	XsDataPacket packet;
	if (retrievePacket(packet))
	{
		gotoXY(x, y);
		out << "Sample counter: " << packet.packetId() << "\n";
		numberOfLines += 2;
		numberOfLines += streamPacket(out, packet);
	}

	return numberOfLines;
}

//--------------------------------------------------------------------------------
bool MTixDisplayer::handleKeyPress(int key)
{
	switch (key)
	{
	case 'a':
		setOrientationEuler();
		break;
	case 's':
		setOrientationQuaternion();
		break;
	case 'd':
		setOrientationMatrix();
		break;
	case 'g':
		setRawData();
		break;
	default:
		return MTDisplayer::handleKeyPress(key);
	}
	return true;
}

//--------------------------------------------------------------------------------
void MTixDisplayer::setConfig(XsDeviceMode const & mode)
{
	XsDevice& device = getDevice();

    XsDeviceState oldState = device.deviceState();

    if (device.isMasterDevice())
    {
        device.gotoConfig();
    }
    else
    {
        device.master()->gotoConfig();
    }

	device.setDeviceMode(mode);
	if (oldState == XDS_Measurement)
	{
		device.gotoMeasurement();
	}
}

//--------------------------------------------------------------------------------
void MTixDisplayer::setOrientationEuler()
{
	XsDeviceMode deviceMode(100);
	deviceMode.setSettingsFlag(XOS_OrientationMode_Euler);
	deviceMode.setModeFlag(XOM_Orientation);
	setConfig(deviceMode);

	MTDisplayer::setOrientationEuler();
}

//--------------------------------------------------------------------------------
void MTixDisplayer::setOrientationQuaternion()
{
	XsDeviceMode deviceMode(100);
	deviceMode.setSettingsFlag(XOS_OrientationMode_Quaternion);
	deviceMode.setModeFlag(XOM_Orientation);
	setConfig(deviceMode);

	MTDisplayer::setOrientationQuaternion();
}

//--------------------------------------------------------------------------------
void MTixDisplayer::setOrientationMatrix()
{
	XsDeviceMode deviceMode(100);
	deviceMode.setSettingsFlag(XOS_OrientationMode_Matrix);
	deviceMode.setModeFlag(XOM_Orientation);
	setConfig(deviceMode);

	MTDisplayer::setOrientationMatrix();
}

//--------------------------------------------------------------------------------
void MTixDisplayer::setRawData()
{
	XsDeviceMode deviceMode(100);
	deviceMode.setSettingsFlag(XOS_Dataformat_Float);
	deviceMode.setModeFlag(XOM_Raw);
	setConfig(deviceMode);

	MTDisplayer::setRawData();
}

//--------------------------------------------------------------------------------

}

#endif
