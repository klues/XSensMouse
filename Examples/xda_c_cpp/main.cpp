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

#include <xsensdeviceapi.h>
#include <xsens/xsmatrix3x3.h>
#include <xsens/xstime.h>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <iomanip>
#include <sstream>
#include "conio.h"
#include "console.h"

#include "xspacketstreamer.h"
#include "xsdevicecallbackhandler.h"
#include "xsportinfostreamer.h"
#include "xsdevicedetector.h"

static void waitForKeyPress();

int main()
{
	clearScreen();

	// Create XsControl
	XsControl* control = XsControl::construct(); assert(control != 0);

	// Create device detector
	XsDemo::XsDeviceDetector deviceDetector(*control);

	// Scan for and open devices
	std::cout << "Detecting devices...\n" << std::string(79, '-') << "\n";
	if (!deviceDetector.detectAndOpenDevices())
	{
		std::cout << "An error occured while detecting devices. Aborting.\n";
		waitForKeyPress();
		return EXIT_FAILURE;
	}
	if (deviceDetector.detectedDevices().empty())
	{
		std::cout << "No devices found. Aborting.\n";
		waitForKeyPress();
		return EXIT_FAILURE;
	}

	for (XsDemo::XsDeviceDetector::DetectedDevices::const_iterator i = deviceDetector.detectedDevices().begin(); i != deviceDetector.detectedDevices().end(); ++i)
	{
		(*i)->gotoMeasurement();
	}

	// Main loop
	bool running = true;
	bool settingChanged = true;
	size_t currentSelectedDeviceIdx = 0;
	XsDevice* currentSelectedDevice = 0;
	XsDemo::IXsMTDisplayer* currentDisplayer = 0;

	while(running)
	{
		if (settingChanged)
		{
			//XsDevice* previousSelectedDevice = currentSelectedDevice;
		    currentSelectedDevice = deviceDetector.detectedDevices()[currentSelectedDeviceIdx];
			XsDemo::XsDeviceDetector::DeviceInfo::const_iterator foundDisplayer = deviceDetector.deviceInfo().find(currentSelectedDevice);
			assert(foundDisplayer != deviceDetector.deviceInfo().end());
			currentDisplayer = foundDisplayer->second.second;

			clearScreen();
			gotoXY(0, 0);
			std::cout << "Detected devices:\n";
			static std::string line = std::string(79,'-');
			std::ostringstream selectDeviceLine;
			std::cout << line << "\n";
			XsSize deviceNumber = 0;
			for (XsDemo::XsDeviceDetector::DetectedDevices::const_iterator i = deviceDetector.detectedDevices().begin(); i != deviceDetector.detectedDevices().end(); ++i)
			{
				std::cout << "Nr " << std::right << std::setw(2) << ++deviceNumber << ": ";
				if (i != deviceDetector.detectedDevices().begin()) {selectDeviceLine << ", ";}
				XsDevice* device = *i;
				assert(device != 0);
				std::cout << std::left << std::setw(15) << (device->productCode().toStdString()) << " ";
				XsDemo::XsDeviceIdStreamer().stream(std::cout, device->deviceId());
				std::cout << " @ ";

				XsPortInfo const * portInfo = deviceDetector.getPort(device);
				assert(portInfo != 0);
				XsDemo::XsPortInfoStreamer().stream(std::cout, *portInfo);

				selectDeviceLine << "'" << deviceNumber << "'";
				if (deviceNumber - 1 == currentSelectedDeviceIdx)
				{
					std::cout << " <----";
				}
				std::cout << "\n";
			}
			std::cout << line << "\n";

			currentDisplayer->displayKeyHelpLine(0, (int)deviceDetector.detectedDevices().size() + 4, std::cout);

			gotoXY(0, (int)deviceDetector.detectedDevices().size() + 5);
			std::cout << "Press " << selectDeviceLine.str() << " to select corresponding device.\n";
			std::cout << "Press 'q' to quit.\n";

			settingChanged = false;
		}

		assert(currentDisplayer != 0);
		currentDisplayer->displayPacket(0, (int)deviceDetector.detectedDevices().size() + 8, std::cout);

		if (_kbhit())
		{
			settingChanged = true;
			int key = _getch();
			if (key >= '1' && key <= '9')
			{
				size_t idx = (key - 48) - 1;
				if (idx < deviceDetector.detectedDevices().size())
				{
					currentSelectedDeviceIdx = idx;
				}
			}
			else switch (key)
			{
			case 'q':
				running = false;
				break;
			default:
				currentDisplayer->handleKeyPress(key);
				break;
			}

			while (_kbhit()) {_getch();}
		}
		XsTime::msleep(0);
	}

	clearScreen();

	// Close devices (do this before destructing the XsControl instance because all references to devices will be invalidated then
	deviceDetector.closeDevices();

	// Close ports
	for (XsDemo::XsDeviceDetector::DetectedPorts::const_iterator i = deviceDetector.detectedPorts().begin(); i != deviceDetector.detectedPorts().end(); ++i)
	{
		control->closePort((*i)->portName());
	}

	// Free XsControl
	control->destruct();

    std::cout << "Successful exit." << std::endl;

	waitForKeyPress();

	return EXIT_SUCCESS;
}

/*! \brief Halts execution until a key is pressed
*/
static void waitForKeyPress()
{
	printf("\nPress a key to continue\n");
	_getch();
}
