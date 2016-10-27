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

#ifndef PACKET_XS_DEVICEDETECTOR_H
#define PACKET_XS_DEVICEDETECTOR_H

#include <set>
#include <map>
#include <xsensdeviceapi.h>

#include "xsmtdisplayer.h"
#include "xsmtixdisplayer.h"
#include "xsmtmk4displayer.h"
#include "xsdevicecallbackhandler.h"

namespace XsDemo
{

//--------------------------------------------------------------------------------
class XsDeviceDetector
{
public:
	typedef std::set<XsPortInfo const *> DetectedPorts;
	typedef std::map<XsDevice*, std::pair<XsPortInfo const *, IXsMTDisplayer*> > DeviceInfo;
	typedef std::vector<XsDevice*> DetectedDevices;

	void closeDevices();

	XsDeviceDetector(XsControl& control) : m_control(control)
	{
	}

	virtual ~XsDeviceDetector()
	{
	}

	bool detectAndOpenDevices();
	DetectedPorts const & detectedPorts() const {return m_detectedPorts;}
	DeviceInfo const & deviceInfo() const {return m_deviceInfo;}
	DetectedDevices const & detectedDevices() const {return m_DetectedDevices;}
	XsPortInfo const * getPort(XsDevice* device) const;

private:
	IXsMTDisplayer* addDevice(XsPortInfo const & portInfo, XsDevice* device);

	XsControl& m_control;
	XsPortInfoArray m_PortInfoArray;

	DetectedPorts m_detectedPorts;
	DeviceInfo m_deviceInfo;
	DetectedDevices m_DetectedDevices;

	XsDeviceDetector(XsDeviceDetector const &);
	XsDeviceDetector& operator = (XsDeviceDetector const &);
};

//--------------------------------------------------------------------------------
XsPortInfo const * XsDeviceDetector::getPort(XsDevice* device) const
{
	XsPortInfo const * result = 0;
	DeviceInfo::const_iterator found = m_deviceInfo.find(device);
	if (found != m_deviceInfo.end())
	{
		result = found->second.first;
	}
	return result;
}

//--------------------------------------------------------------------------------
IXsMTDisplayer* XsDeviceDetector::addDevice(XsPortInfo const & portInfo, XsDevice* device)
{
	IXsMTDisplayer* addedDisplayer = 0;

	assert(device != 0);
	m_detectedPorts.insert(&portInfo);
	XsDeviceId const & deviceId = device->deviceId();
	m_DetectedDevices.push_back(device);
	if (deviceId.isMt9c() || deviceId.isLegacyMtig())
	{
		m_deviceInfo.insert(DeviceInfo::value_type(device, std::make_pair(&portInfo, addedDisplayer = new XsDemo::MTixDisplayer(portInfo, *device))));
	}
	else if (deviceId.isMtMk4() || deviceId.isFmt_X000())
	{
		m_deviceInfo.insert(DeviceInfo::value_type(device, std::make_pair(&portInfo, addedDisplayer = new XsDemo::MTmk4Displayer(portInfo, *device))));
	}
	else
	{
		m_deviceInfo.insert(DeviceInfo::value_type(device, std::make_pair(&portInfo, addedDisplayer = new XsDemo::MTUnknownDisplayer(portInfo, *device))));
	}

	assert(addedDisplayer != 0);
	return addedDisplayer;
}

//--------------------------------------------------------------------------------
bool XsDeviceDetector::detectAndOpenDevices()
{
	m_PortInfoArray = XsScanner::scanPorts();

	size_t deviceCounter = 0;
	for (XsPortInfoArray::const_iterator portInfoIter = m_PortInfoArray.begin(); portInfoIter != m_PortInfoArray.end(); ++portInfoIter)
	{
		++deviceCounter;
		if (!m_control.openPort(portInfoIter->portName(), portInfoIter->baudrate()))
		{
			return false;
		}

		XsPortInfo const & portInfo = *portInfoIter;
		XsDevice* detectedDevice = m_control.device(portInfoIter->deviceId());
		addDevice(portInfo, detectedDevice);
	}

	return true;
}

//--------------------------------------------------------------------------------
void XsDeviceDetector::closeDevices()
{
	for (DeviceInfo::iterator i = m_deviceInfo.begin(); i != m_deviceInfo.end(); ++i)
	{
		delete i->second.second;
	}
}

//--------------------------------------------------------------------------------

}

#endif
