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

//--------------------------------------------------------------------------------
// Xsens device API example for an MTi / MTx / Mtmk4 device using the C++ API
//
//--------------------------------------------------------------------------------
#include <xsensdeviceapi.h> // The Xsens device API header

#include <iostream>
#include <list>
#include <iomanip>
#include <stdexcept>

#include <xsens/xstime.h>
#include <xsens/xsmutex.h>

#include "conio.h" // for non ANSI _kbhit() and _getch()

//--------------------------------------------------------------------------------
// CallbackHandler is an object derived from XsCallback that can be attached to
// an XsDevice using the XsDevice::setCallbackHandler method.
// Various virtual methods which are automatically called by the XsDevice can be
// overridden (See XsCallback)
// Only the onPostProcess(...) method is overridden here. This method is called
// when a new data packet is available.
// Note that this method will be called from within the thread of the XsDevice so
// proper synchronisation is required.
// It is recommended to keep the implementation of these methods fast; therefore
// the only action here is to copy the packet to a queue where it can be
// retrieved later by the main thread to be displayed. All access to the queue is
// protected by a critical section because multiple threads are accessing it.
//--------------------------------------------------------------------------------
class CallbackHandler : public XsCallback
{
public:
	CallbackHandler(size_t maxBufferSize = 5) :
		m_maxNumberOfPacketsInBuffer(maxBufferSize),
		m_numberOfPacketsInBuffer(0)
	{
	}

	virtual ~CallbackHandler() throw()
	{
	}

	bool packetAvailable() const
	{
		XsMutexLocker lock(m_mutex);
		return m_numberOfPacketsInBuffer > 0;
	}

	XsDataPacket getNextPacket()
	{
		assert(packetAvailable());
		XsMutexLocker lock(m_mutex);
		XsDataPacket oldestPacket(m_packetBuffer.front());
		m_packetBuffer.pop_front();
		--m_numberOfPacketsInBuffer;
		return oldestPacket;
	}

protected:
	virtual void onLiveDataAvailable(XsDevice*, const XsDataPacket* packet)
	{
		XsMutexLocker lock(m_mutex);
		assert(packet != 0);
		while (m_numberOfPacketsInBuffer >= m_maxNumberOfPacketsInBuffer)
		{
			(void)getNextPacket();
		}
		m_packetBuffer.push_back(*packet);
		++m_numberOfPacketsInBuffer;
		assert(m_numberOfPacketsInBuffer <= m_maxNumberOfPacketsInBuffer);
	}
private:
	mutable XsMutex m_mutex;

	size_t m_maxNumberOfPacketsInBuffer;
	size_t m_numberOfPacketsInBuffer;
	std::list<XsDataPacket> m_packetBuffer;
};

//--------------------------------------------------------------------------------
int main(void)
{
	// Create XsControl object
	std::cout << "Creating XsControl object..." << std::endl;
	XsControl* control = XsControl::construct();
	assert(control != 0);

	try
	{
		// Scan for connected devices
		std::cout << "Scanning for devices..." << std::endl;
		XsPortInfoArray portInfoArray = XsScanner::scanPorts();

		// Find an MTi / MTx / MTmk4 device
		XsPortInfoArray::const_iterator mtPort = portInfoArray.begin();
		while (mtPort != portInfoArray.end() && !mtPort->deviceId().isMt9c() && !mtPort->deviceId().isLegacyMtig() && !mtPort->deviceId().isMtMk4() && !mtPort->deviceId().isFmt_X000()) {++mtPort;}
		if (mtPort == portInfoArray.end())
		{
			throw std::runtime_error("No MTi / MTx / MTmk4 device found. Aborting.");
		}
		std::cout << "Found a device with id: " << mtPort->deviceId().toString().toStdString() << " @ port: " << mtPort->portName().toStdString() << ", baudrate: " << mtPort->baudrate() << std::endl;

		// Open the port with the detected device
		std::cout << "Opening port..." << std::endl;
		if (!control->openPort(mtPort->portName().toStdString(), mtPort->baudrate()))
		{
			throw std::runtime_error("Could not open port. Aborting.");
		}

		try
		{
			// Get the device object
			XsDevice* device = control->device(mtPort->deviceId());
			assert(device != 0);

			// Print information about detected MTi / MTx / MTmk4 device
			std::cout << "Device: " << device->productCode().toStdString() << " opened." << std::endl;

			// Create and attach callback handler to device
			CallbackHandler callback;
			device->addCallbackHandler(&callback);

			// Put the device in configuration mode
			std::cout << "Putting device into configuration mode..." << std::endl;
			if (!device->gotoConfig()) // Put the device into configuration mode before configuring the device
			{
				throw std::runtime_error("Could not put device into configuration mode. Aborting.");
			}

			// Configure the device. Note the differences between MTix and MTmk4
			std::cout << "Configuring the device..." << std::endl;
			if (device->deviceId().isMt9c() || device->deviceId().isLegacyMtig())
			{
				XsOutputMode outputMode = XOM_Orientation; // output orientation data
				XsOutputSettings outputSettings = XOS_OrientationMode_Quaternion; // output orientation data as quaternion
				XsDeviceMode deviceMode(100); // make a device mode with update rate: 100 Hz
				deviceMode.setModeFlag(outputMode);
				deviceMode.setSettingsFlag(outputSettings);

				// set the device configuration
				if (!device->setDeviceMode(deviceMode))
				{
					throw std::runtime_error("Could not configure MTmki device. Aborting.");
				}
			}
			else if (device->deviceId().isMtMk4() || mtPort->deviceId().isFmt_X000())
			{
				XsOutputConfiguration quat(XDI_Quaternion, 0);
				XsOutputConfigurationArray configArray;
				configArray.push_back(quat);
				if (!device->setOutputConfiguration(configArray))
				{

					throw std::runtime_error("Could not configure MTmk4 device. Aborting.");
				}
			}
			else
			{
				throw std::runtime_error("Unknown device while configuring. Aborting.");
			}

			// Put the device in measurement mode
			std::cout << "Putting device into measurement mode..." << std::endl;
			if (!device->gotoMeasurement())
			{
				throw std::runtime_error("Could not put device into measurement mode. Aborting.");
			}

			std::cout << "\nMain loop (press any key to quit)" << std::endl;
			std::cout << std::string(79, '-') << std::endl;
			while (!_kbhit())
			{
				if (callback.packetAvailable())
				{
					// Retrieve a packet
					XsDataPacket packet = callback.getNextPacket();

					// Get the quaternion data
					XsQuaternion quaternion = packet.orientationQuaternion();
					std::cout << "\r"
							  << "q0:" << std::setw(5) << std::fixed << std::setprecision(2) << quaternion.w()
							  << ",q1:" << std::setw(5) << std::fixed << std::setprecision(2) << quaternion.x()
							  << ",q2:" << std::setw(5) << std::fixed << std::setprecision(2) << quaternion.y()
							  << ",q3:" << std::setw(5) << std::fixed << std::setprecision(2) << quaternion.z()
					;

					// Convert packet to euler
					XsEuler euler = packet.orientationEuler();
					std::cout << ",Roll:" << std::setw(7) << std::fixed << std::setprecision(2) << euler.roll()
							  << ",Pitch:" << std::setw(7) << std::fixed << std::setprecision(2) << euler.pitch()
							  << ",Yaw:" << std::setw(7) << std::fixed << std::setprecision(2) << euler.yaw()
					;

					std::cout << std::flush;
				}
				XsTime::msleep(0);
			}
			_getch();
			std::cout << "\n" << std::string(79, '-') << "\n";
			std::cout << std::endl;
		}
		catch (std::runtime_error const & error)
		{
			std::cout << error.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "An unknown fatal error has occured. Aborting." << std::endl;
		}

		// Close port
		std::cout << "Closing port..." << std::endl;
		control->closePort(mtPort->portName().toStdString());
	}
	catch (std::runtime_error const & error)
	{
		std::cout << error.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "An unknown fatal error has occured. Aborting." << std::endl;
	}

	// Free XsControl object
	std::cout << "Freeing XsControl object..." << std::endl;
	control->destruct();

	std::cout << "Successful exit." << std::endl;

	std::cout << "Press [ENTER] to continue." << std::endl; std::cin.get();

	return 0;
}
