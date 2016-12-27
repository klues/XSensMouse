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

#include <xsensdeviceapi.h> // The Xsens device API header
#include "conio.h"			// For non ANSI _kbhit() and _getch()

#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
#include <list>
#include <utility>
#include <iostream>
#include <fstream>
using namespace std;


#include <xsens/xsmutex.h>

float MOUSE_SPEED = 5;
int N_AVG = 7;
float thresholdClick = 2.5, thresholdClickEnd = 0.15, thresholdIgnore = 1.5;
float offsetX, offsetY;

/*! \brief Stream insertion operator overload for XsPortInfo */
std::ostream& operator << (std::ostream& out, XsPortInfo const & p)
{
	out << "Port: " << std::setw(2) << std::right << p.portNumber() << " (" << p.portName().toStdString() << ") @ "
		<< std::setw(7) << p.baudrate() << " Bd"
		<< ", " << "ID: " << p.deviceId().toString().toStdString()
		;
	return out;
}

/*! \brief Stream insertion operator overload for XsDevice */
std::ostream& operator << (std::ostream& out, XsDevice const & d)
{
	out << "ID: " << d.deviceId().toString().toStdString() << " (" << d.productCode().toStdString() << ")";
	return out;
}

/*! \brief Given a list of update rates and a desired update rate, returns the closest update rate to the desired one */
int findClosestUpdateRate(const XsIntArray& supportedUpdateRates, const int desiredUpdateRate)
{
	if (supportedUpdateRates.empty())
	{
		return 0;
	}

	if (supportedUpdateRates.size() == 1)
	{
		return supportedUpdateRates[0];
	}

	int uRateDist = -1;
	int closestUpdateRate = -1;
	for (XsIntArray::const_iterator itUpRate = supportedUpdateRates.begin(); itUpRate != supportedUpdateRates.end(); ++itUpRate)
	{
		const int currDist = std::abs(*itUpRate - desiredUpdateRate);

		if ((uRateDist == -1) || (currDist < uRateDist))
		{
			uRateDist = currDist;
			closestUpdateRate = *itUpRate;
		}
	}
	return closestUpdateRate;
}

//----------------------------------------------------------------------
// Callback handler for wireless master
//----------------------------------------------------------------------
class WirelessMasterCallback : public XsCallback
{
public:
	typedef std::set<XsDevice*> XsDeviceSet;

	XsDeviceSet getWirelessMTWs() const
	{
		XsMutexLocker lock(m_mutex);
		return m_connectedMTWs;
	}

protected:
	virtual void onConnectivityChanged(XsDevice* dev, XsConnectivityState newState)
	{
		XsMutexLocker lock(m_mutex);
		switch (newState)
		{
		case XCS_Disconnected:		/*!< Device has disconnected, only limited informational functionality is available. */
			std::cout << "\nEVENT: MTW Disconnected -> " << *dev << std::endl;
			m_connectedMTWs.erase(dev);
			break;
		case XCS_Rejected:			/*!< Device has been rejected and is disconnected, only limited informational functionality is available. */
			std::cout << "\nEVENT: MTW Rejected -> " << *dev << std::endl;
			m_connectedMTWs.erase(dev);
			break;
		case XCS_PluggedIn:			/*!< Device is connected through a cable. */
			std::cout << "\nEVENT: MTW PluggedIn -> " << *dev << std::endl;
			m_connectedMTWs.erase(dev);
			break;
		case XCS_Wireless:			/*!< Device is connected wirelessly. */
			std::cout << "\nEVENT: MTW Connected -> " << *dev << std::endl;
			m_connectedMTWs.insert(dev);
			break;
		case XCS_File:				/*!< Device is reading from a file. */
			std::cout << "\nEVENT: MTW File -> " << *dev << std::endl;
			m_connectedMTWs.erase(dev);
			break;
		case XCS_Unknown:			/*!< Device is in an unknown state. */
			std::cout << "\nEVENT: MTW Unknown -> " << *dev << std::endl;
			m_connectedMTWs.erase(dev);
			break;
		default:
			std::cout << "\nEVENT: MTW Error -> " << *dev << std::endl;
			m_connectedMTWs.erase(dev);
			break;
		}
	}
private:
	mutable XsMutex m_mutex;
	XsDeviceSet m_connectedMTWs;
};

//----------------------------------------------------------------------
// Callback handler for MTw
// Handles onDataAvailable callbacks for MTW devices
//----------------------------------------------------------------------
class MtwCallback : public XsCallback
{
public:
	MtwCallback(int mtwIndex, XsDevice* device)
		:m_mtwIndex(mtwIndex)
		, m_device(device)
	{}

	bool dataAvailable() const
	{
		XsMutexLocker lock(m_mutex);
		return !m_packetBuffer.empty();
	}

	XsDataPacket const * getOldestPacket() const
	{
		XsMutexLocker lock(m_mutex);
		XsDataPacket const * packet = &m_packetBuffer.front();
		return packet;
	}

	void deleteOldestPacket()
	{
		XsMutexLocker lock(m_mutex);
		m_packetBuffer.pop_front();
	}

	int getMtwIndex() const
	{
		return m_mtwIndex;
	}

	XsDevice const & device() const
	{
		assert(m_device != 0);
		return *m_device;
	}

protected:
	virtual void onLiveDataAvailable(XsDevice*, const XsDataPacket* packet)
	{
		XsMutexLocker lock(m_mutex);
		// NOTE: Processing of packets should not be done in this thread.

		m_packetBuffer.push_back(*packet);
		if (m_packetBuffer.size() > 300)
		{
			std::cout << std::endl;
			deleteOldestPacket();
		}
	}

private:
	mutable XsMutex m_mutex;
	std::list<XsDataPacket> m_packetBuffer;
	int m_mtwIndex;
	XsDevice* m_device;
};

void mouseLeftClick();

float movingAvg(float oldAvg, float newValue);

int sign(float f);

//limits a value to an maximum absolute value
//eg. limitAbs(-2.5, 1.5) == -1.5
float limitAbs(float f, float maxAbs);

float limitValue(float value, float minValue, float maxValue);

//starts measurement and mouse movement
void startMouse(std::vector<MtwCallback*> mtwCallbacks, bool calibrate);

//----------------------------------------------------------------------
// Main
//----------------------------------------------------------------------
int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;
	const int desiredUpdateRate = 75;	// Use 75 Hz update rate for MTWs
	const int desiredRadioChannel = 19;	// Use radio channel 19 for wireless master.

	WirelessMasterCallback wirelessMasterCallback; // Callback for wireless master
	std::vector<MtwCallback*> mtwCallbacks; // Callbacks for mtw devices

	//std::cout << "Constructing XsControl..." << std::endl;
	XsControl* control = XsControl::construct();
	if (control == 0)
	{
		std::cout << "Failed to construct XsControl instance." << std::endl;
	}

	try
	{
		//std::cout << "Scanning ports..." << std::endl;
		XsPortInfoArray detectedDevices = XsScanner::scanPorts();
		std::cout << "------------------" << std::endl;
		std::cout << "--- XSens Mouse---" << std::endl;
		std::cout << "------------------" << std::endl;
		std::cout << "Starting Hardware..." << std::endl;
		std::cout << "Finding USB-Dongle..." << std::endl;
		XsPortInfoArray::const_iterator wirelessMasterPort = detectedDevices.begin();
		while (wirelessMasterPort != detectedDevices.end() && !wirelessMasterPort->deviceId().isWirelessMaster())
		{
			++wirelessMasterPort;
		}
		if (wirelessMasterPort == detectedDevices.end())
		{
			throw std::runtime_error("No wireless masters found");
		}
		std::cout << "USB-Dongle found @ " << *wirelessMasterPort << std::endl;

		//std::cout << "Opening port..." << std::endl;
		if (!control->openPort(wirelessMasterPort->portName().toStdString(), wirelessMasterPort->baudrate()))
		{
			std::ostringstream error;
			error << "Failed to open port " << *wirelessMasterPort;
			throw std::runtime_error(error.str());
		}

		//std::cout << "Getting XsDevice instance for wireless master..." << std::endl;
		XsDevicePtr wirelessMasterDevice = control->device(wirelessMasterPort->deviceId());
		if (wirelessMasterDevice == 0)
		{
			std::ostringstream error;
			error << "Failed to construct XsDevice instance: " << *wirelessMasterPort;
			throw std::runtime_error(error.str());
		}

		//std::cout << "XsDevice instance created @ " << *wirelessMasterDevice << std::endl;

		//std::cout << "Setting config mode..." << std::endl;
		if (!wirelessMasterDevice->gotoConfig())
		{
			std::ostringstream error;
			error << "Failed to goto config mode: " << *wirelessMasterDevice;
			throw std::runtime_error(error.str());
		}

		//std::cout << "Attaching callback handler..." << std::endl;
		wirelessMasterDevice->addCallbackHandler(&wirelessMasterCallback);

		//std::cout << "Getting the list of the supported update rates..." << std::endl;
		const XsIntArray supportedUpdateRates = wirelessMasterDevice->supportedUpdateRates();

		//std::cout << "Supported update rates: ";
		for (XsIntArray::const_iterator itUpRate = supportedUpdateRates.begin(); itUpRate != supportedUpdateRates.end(); ++itUpRate)
		{
			std::cout << *itUpRate << " ";
		}
		std::cout << std::endl;

		const int newUpdateRate = findClosestUpdateRate(supportedUpdateRates, desiredUpdateRate);

		//std::cout << "Setting update rate to " << newUpdateRate << " Hz..." << std::endl;
		if (!wirelessMasterDevice->setUpdateRate(newUpdateRate))
		{
			std::ostringstream error;
			error << "Failed to set update rate: " << *wirelessMasterDevice;
			throw std::runtime_error(error.str());
		}

		//std::cout << "Disabling radio channel if previously enabled..." << std::endl;
		if (wirelessMasterDevice->isRadioEnabled())
		{
			if (!wirelessMasterDevice->disableRadio())
			{
				std::ostringstream error;
				error << "Failed to disable radio channel: " << *wirelessMasterDevice;
				throw std::runtime_error(error.str());
			}
		}

		//std::cout << "Setting radio channel to " << desiredRadioChannel << " and enabling radio..." << std::endl;
		if (!wirelessMasterDevice->enableRadio(desiredRadioChannel))
		{
			std::ostringstream error;
			error << "Failed to set radio channel: " << *wirelessMasterDevice;
			throw std::runtime_error(error.str());
		}

		std::cout << "Waiting for XSens to wirelessly connect..." << std::endl;

		bool waitForConnections = true;
		size_t connectedMTWCount = wirelessMasterCallback.getWirelessMTWs().size();
		do
		{
			XsTime::msleep(100);

			while (true)
			{
				size_t nextCount = wirelessMasterCallback.getWirelessMTWs().size();
				if (nextCount != connectedMTWCount)
				{
					std::cout << "XSens connected! Mouse is ready to start." << std::endl << std::endl;
					
					std::cout << "Configuration of mouse is possible during usage. Press:" << std::endl;
					std::cout << "<s> for changing mouse speed." << std::endl;
					std::cout << "<m> for changing mouse smoothing." << std::endl;
					std::cout << "<t> for changing click threshold." << std::endl;
					std::cout << "<ESC> to exit the program" << std::endl << std::endl;
					std::cout << "while the mouse is running." << std::endl;
					std::cout << "Press any key to start mouse." << std::endl;
					connectedMTWCount = nextCount;
				}
				else
				{
					break;
				}
			}
			if (_kbhit())
			{
				(void)_getch();
				waitForConnections = false;
			}
		} while (waitForConnections);

		std::cout << "Starting mouse..." << std::endl;
		if (!wirelessMasterDevice->gotoMeasurement())
		{
			std::ostringstream error;
			error << "Failed to goto measurement mode: " << *wirelessMasterDevice;
			throw std::runtime_error(error.str());
		}

		XsDeviceIdArray allDeviceIds = control->deviceIds();
		XsDeviceIdArray mtwDeviceIds;
		for (XsDeviceIdArray::const_iterator i = allDeviceIds.begin(); i != allDeviceIds.end(); ++i)
		{
			if (i->isMtw())
			{
				mtwDeviceIds.push_back(*i);
			}
		}
		XsDevicePtrArray mtwDevices;
		for (XsDeviceIdArray::const_iterator i = mtwDeviceIds.begin(); i != mtwDeviceIds.end(); ++i)
		{
			XsDevicePtr mtwDevice = control->device(*i);
			if (mtwDevice != 0)
			{
				mtwDevices.push_back(mtwDevice);
			}
			else
			{
				throw std::runtime_error("Failed to create an MTW XsDevice instance");
			}
		}

		mtwCallbacks.resize(mtwDevices.size());
		for (int i = 0; i < (int)mtwDevices.size(); ++i)
		{
			mtwCallbacks[i] = new MtwCallback(i, mtwDevices[i]);
			mtwDevices[i]->addCallbackHandler(mtwCallbacks[i]);
		}

		char character = 'c';
		char lastC = 0;
		while ((int) character != 27) { //ESC == 27
			
			if (character == '+' || character == '-') {
				if (lastC == 'm') { //change number for moving AVG
					int sign = 1;
					if (character == '-') sign = -1;
					int newVal = N_AVG + sign;
					if (newVal < 21 && newVal > 0) {
						std::cout << "set smoothing from " << N_AVG << " to " << newVal << std::endl;
						N_AVG = newVal;
					}
				} else if (lastC == 's') { //change mouse speed
					int sign = 1;
					if (character == '-') sign = -1;
					int newVal = MOUSE_SPEED + sign;
					if (newVal < 21 && newVal > 0) {
						std::cout << "set speed from " << MOUSE_SPEED << " to " << newVal << std::endl;
						MOUSE_SPEED = newVal;
					}
				} else if (lastC == 't') { //change mouse speed
					float factor = 1.1;
					if (character == '-') factor = 0.9;
					float newVal = thresholdClick * factor;
					if (newVal < 5.0 && newVal > 1.0) {
						std::cout << "set threshold from " << thresholdClick << " to " << newVal << std::endl;
						thresholdClick = newVal;
					}
				}
			}

			if (character == 'm') {
				std::cout << "changing mouse s<m>oothing, press <+> to increase and <-> to decrease (max: 20, min: 1)..." << std::endl;
			} else if (character == 's') {
				std::cout << "changing mouse <s>peed, press <+> to increase and <-> to decrease (max: 20, min: 1)..." << std::endl;
			} else if (character == 't') {
				std::cout << "changing mouse click <t>reshold, press <+> to increase and <-> to decrease (max: 5.0, min: 1.0)..." << std::endl;
			}

			if (character == 'c') { //calibrate
				std::cout << "calibrating mouse... " << std::endl;
				startMouse(mtwCallbacks, true);
			}
			else {
				startMouse(mtwCallbacks, false);
			}
			
			if (character != '+' && character != '-') {
				lastC = character;
			}
			character = _getch();
		}

		//std::cout << "Setting config mode..." << std::endl;
		if (!wirelessMasterDevice->gotoConfig())
		{
			std::ostringstream error;
			error << "Failed to goto config mode: " << *wirelessMasterDevice;
			throw std::runtime_error(error.str());
		}

		//std::cout << "Disabling radio... " << std::endl;
		if (!wirelessMasterDevice->disableRadio())
		{
			std::ostringstream error;
			error << "Failed to disable radio: " << *wirelessMasterDevice;
			throw std::runtime_error(error.str());
		}
	}
	catch (std::exception const & ex)
	{
		std::cout << ex.what() << std::endl;
		std::cout << "****ABORT****" << std::endl;
	}
	catch (...)
	{
		std::cout << "An unknown fatal error has occured. Aborting." << std::endl;
		std::cout << "****ABORT****" << std::endl;
	}

	//std::cout << "Closing XsControl..." << std::endl;
	control->close();

	//std::cout << "Deleting mtw callbacks..." << std::endl;
	for (std::vector<MtwCallback*>::iterator i = mtwCallbacks.begin(); i != mtwCallbacks.end(); ++i)
	{
		delete (*i);
	}

	std::cout << "Successful exit." << std::endl;
	return 0;
}

void mouseLeftClick()
{
	INPUT Input = { 0 };

	// left down
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	::SendInput(1, &Input, sizeof(INPUT));

	// left up
	::ZeroMemory(&Input, sizeof(INPUT));
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	::SendInput(1, &Input, sizeof(INPUT));
}

float movingAvg(float oldAvg, float newValue) {
	oldAvg -= oldAvg / N_AVG;
	return oldAvg + newValue / N_AVG * 1.0;
}

int sign(float f) {
	if (f >= 0) return 1;
	return -1;
}

float limitAbs(float f, float maxAbs) {
	if (abs(f) <= maxAbs) return f;
	return maxAbs * sign(f);
}

float limitValue(float value, float minValue, float maxValue) {
	if (value < minValue) {
		return minValue;
	}
	else if (value > maxValue) {
		return maxValue;
	}
	return value;
}

void startMouse(std::vector<MtwCallback*> mtwCallbacks, bool calibrate) {
	std::vector<XsEuler> eulerData(mtwCallbacks.size()); // Room to store euler data for each mtw
	XsVector velocity, rotation;
	//ofstream fileXyZ;
	//fileXyZ.open("dataAcc-front-back.csv");
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	float cursorX = cursorPos.x;
	float cursorY = cursorPos.y;
	float normValueX = 0, normValueY = 0;
	bool inclick = false;
	int calibrateRuns = 0;
	if (calibrate) {
		calibrateRuns = N_AVG;
	}
	DWORD displayWidth = GetSystemMetrics(SM_CXSCREEN);
	DWORD displayHeight = GetSystemMetrics(SM_CYSCREEN);

	while (!_kbhit()) {
		XsTime::msleep(0);

		bool newDataAvailable = false;
		for (size_t i = 0; i < mtwCallbacks.size(); ++i)
		{
			if (mtwCallbacks[i]->dataAvailable())
			{
				newDataAvailable = true;
				XsDataPacket const * packet = mtwCallbacks[i]->getOldestPacket();
				eulerData[i] = packet->orientationEuler();
				velocity = packet->calibratedAcceleration();
				rotation = packet->calibratedMagneticField();
				mtwCallbacks[i]->deleteOldestPacket();
			}
		}

		if (newDataAvailable)
		{
			for (size_t i = 0; i < mtwCallbacks.size(); ++i)
			{
				if (calibrateRuns > 0) {
					if (calibrateRuns == N_AVG) {
						offsetX = velocity.at(1);
						offsetY = velocity.at(0);
					}
					calibrateRuns--;
					offsetX = movingAvg(offsetX, velocity.at(1)); //1
					offsetY = movingAvg(offsetY, velocity.at(0));
					if (calibrateRuns == 1) {
						std::cout << "offset X: " << offsetX << "\n";
						std::cout << "offset Y: " << offsetY << "\n";
						std::cout << "mouse calibrated!" << std::endl;
					}
				}

				float rawNewX = velocity.at(1) - offsetX;
				float rawNewY = velocity.at(0) - offsetY;

				if (abs(rawNewY) < thresholdClickEnd) {
					inclick = false;
				}

				if (!inclick)
				{
					if (abs(rawNewY - normValueY) > thresholdClick) {
						if (rawNewY - normValueY < 0) { //at foot up
							std::cout << "left click!" << "\n";
							mouseLeftClick();
							inclick = true;
						}
					}

					if (abs(rawNewX - normValueX) > thresholdIgnore) {
						rawNewX = limitAbs(rawNewX, thresholdIgnore);
					}
					if (abs(rawNewY - normValueY) > thresholdIgnore) {
						rawNewY = limitAbs(rawNewY, thresholdIgnore);
					}
					normValueX = movingAvg(normValueX, rawNewX);
					normValueY = movingAvg(normValueY, rawNewY);

					if (abs(normValueX) > 0.01 || abs(normValueY) > 0.01) {
						GetCursorPos(&cursorPos);
						cursorY = cursorPos.y - sign(normValueY)*normValueY*normValueY*MOUSE_SPEED;
						cursorX = cursorPos.x - sign(normValueX)*normValueX*normValueX*MOUSE_SPEED;
						cursorY = limitValue(cursorY, 0, displayHeight);
						cursorX = limitValue(cursorX, 0, displayWidth);
						SetCursorPos(cursorX, cursorY);
					}
				}
			}
		}
	}
}