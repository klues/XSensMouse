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

#ifndef PACKET_XS_MTMK4DISPLAYER_H
#define PACKET_XS_MTMK4DISPLAYER_H

#include <xsensdeviceapi.h>
#include "xsmtdisplayer.h"

namespace XsDemo
{

//--------------------------------------------------------------------------------
class MTmk4Displayer : public MTDisplayer
{
public:
	MTmk4Displayer(XsPortInfo const & portInfo, XsDevice& device)
		: MTDisplayer(portInfo, device)
	{
		MTmk4Displayer::setOrientationEuler();
	}

	virtual int displayPacket(int x, int y, std::ostream& out);
	virtual void displayKeyHelpLine(int x, int y, std::ostream& out) const;
	virtual bool handleKeyPress(int key);

protected:
	virtual void setConfig(XsOutputConfigurationArray const &);

	virtual void setOrientationEuler();
	virtual void setOrientationQuaternion();
	virtual void setOrientationMatrix();
	virtual void setOrientationSdi();
	virtual void setRawData();
};

//--------------------------------------------------------------------------------
int MTmk4Displayer::displayPacket(int x, int y, std::ostream& out)
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
void MTmk4Displayer::displayKeyHelpLine(int x, int y, std::ostream& out) const
{
	gotoXY(x, y);
	out << "Press: 'a' : euler, 's': quaternion, 'd' : matrix, 'f' : SDI, 'g' : raw.";
}

//--------------------------------------------------------------------------------
bool MTmk4Displayer::handleKeyPress(int key)
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
	case 'f':
		setOrientationSdi();
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
void MTmk4Displayer::setConfig(XsOutputConfigurationArray const & config)
{
	XsDevice& device = getDevice();

	XsDeviceState oldState = device.deviceState();
	device.gotoConfig();
	XsOutputConfigurationArray configSet(config);
	device.setOutputConfiguration(configSet);
	if (oldState == XDS_Measurement)
	{
		device.gotoMeasurement();
	}
}

//--------------------------------------------------------------------------------
void MTmk4Displayer::setOrientationEuler()
{
	XsOutputConfiguration euler(XDI_EulerAngles, 0);
	XsOutputConfigurationArray configArray;
	configArray.push_back(euler);
	setConfig(configArray);
	MTDisplayer::setOrientationEuler();
}

//--------------------------------------------------------------------------------
void MTmk4Displayer::setOrientationQuaternion()
{
	XsOutputConfiguration quat(XDI_Quaternion, 0);
	XsOutputConfigurationArray configArray;
	configArray.push_back(quat);
	setConfig(configArray);
	MTDisplayer::setOrientationQuaternion();
}

//--------------------------------------------------------------------------------
void MTmk4Displayer::setOrientationMatrix()
{
	XsOutputConfiguration quat(XDI_RotationMatrix, 0);
	XsOutputConfigurationArray configArray;
	configArray.push_back(quat);
	setConfig(configArray);
	MTDisplayer::setOrientationMatrix();
}

//--------------------------------------------------------------------------------
void MTmk4Displayer::setOrientationSdi()
{
	XsOutputConfigurationArray configArray;
	XsOutputConfiguration dq(XDI_DeltaQ, 0);
	XsOutputConfiguration dv(XDI_DeltaV, 0);
	configArray.push_back(dq);
	configArray.push_back(dv);
	setConfig(configArray);
	MTDisplayer::setOrientationSdi();
}

//--------------------------------------------------------------------------------
void MTmk4Displayer::setRawData()
{
	XsOutputConfiguration quat(XDI_RawAccGyrMagTemp, 0);
	XsOutputConfigurationArray configArray;
	configArray.push_back(quat);
	setConfig(configArray);
	MTDisplayer::setRawData();
}

//--------------------------------------------------------------------------------

}

#endif
