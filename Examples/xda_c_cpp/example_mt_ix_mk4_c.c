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
// Xsens device API example for an MTi / MTx / MTmk4 device using the C API
//
//--------------------------------------------------------------------------------
#ifdef __cplusplus
#error Compiling as c++. Please compile this as c to properly test the c api
#endif

#include <xsensdeviceapi.h> // The Xsens device API header
#include <xsens/xstime.h>
#include <xsens/xsmutex.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conio.h" // for non ANSI _kbhit() and _getch()
#include "console.h"

//--------------------------------------------------------------------------------
// PacketQueue is a queue of packets implemented by a linked array.
// Adding is done at the end while removing(reading) is done from the beginning
// When the size becomes larger than the max size, elements are removed from the
// beginning until the size is <= max size.
// All access to the queue is protected by a critical section because multiple
// threads are accessing it (the XsDevice thread and the main thread)
//--------------------------------------------------------------------------------
#define MAX_PACKET_QUEUE_LENGTH 5

typedef struct QueueElement
{
	XsDataPacket m_packet;
	struct QueueElement* m_next;
} QueueElement;

typedef struct PacketQueue
{
	QueueElement* m_first;
	QueueElement* m_last;
	int m_size;
	int m_maxSize;
	XsMutex m_mutex;
} PacketQueue;

PacketQueue packetQueue;

void PacketQueue_init(PacketQueue* queue)
{
	memset(queue, 0, sizeof(PacketQueue));
	XsMutex_construct(&queue->m_mutex);
	queue->m_maxSize = MAX_PACKET_QUEUE_LENGTH;
}

void PacketQueue_destruct(PacketQueue* queue)
{
	while (queue->m_first != NULL)
	{
		QueueElement* p = queue->m_first;
		queue->m_first = queue->m_first->m_next;
		free(p);
	}
	XsMutex_destruct(&queue->m_mutex);
}

BOOL PacketQueue_remove(PacketQueue* queue, XsDataPacket* packet)
{
	BOOL removed = FALSE;
	assert(queue != NULL);
	assert(packet != NULL);

	XsMutex_lock(&queue->m_mutex);
	if (queue->m_size > 0)
	{
		QueueElement* removedElement = queue->m_first;
		XsDataPacket_copy(packet, &removedElement->m_packet);
		queue->m_first = queue->m_first->m_next;
		--queue->m_size;
		if (queue->m_size == 0)
		{
			queue->m_last = queue->m_first;
		}
		
		XsDataPacket_destruct(&removedElement->m_packet);
		free(removedElement);
		removed = TRUE;
	}
	XsMutex_unlock(&queue->m_mutex);
	return removed;
}

void PacketQueue_add(PacketQueue* queue, const XsDataPacket* packet)
{
	QueueElement* pQueueElement = NULL;
	assert(queue != NULL);
	assert(packet != NULL);

	XsMutex_lock(&queue->m_mutex);
	
	pQueueElement = (QueueElement*)malloc(sizeof(QueueElement));
	
	XsDataPacket_construct(&pQueueElement->m_packet);
	XsDataPacket_copy(&pQueueElement->m_packet, packet);
	
	pQueueElement->m_next = NULL;
	if (queue->m_size == 0)
	{
		queue->m_first = pQueueElement;
		queue->m_last = pQueueElement;
		queue->m_size = 1;
	}
	else
	{
		queue->m_last->m_next = pQueueElement;
		queue->m_last = pQueueElement;
		++queue->m_size;
	}

	while (queue->m_size > queue->m_maxSize)
	{
		QueueElement* removedElement = queue->m_first;
		queue->m_first = queue->m_first->m_next;
		queue->m_size--;
		if (queue->m_size == 0)
			queue->m_last = queue->m_first;
		free(removedElement);
	}

	XsMutex_unlock(&queue->m_mutex);
}

//--------------------------------------------------------------------------------
// If the onLiveDataAvailable function assigned to the m_onLiveDataAvailable member
// in an XsCallback struct and that struct is set as callback handler using
// setCallbackHandler then this function is called when a new packet is received.
//
// Note that this method will be called from within the thread of the XsDevice so
// proper synchronisation is required.
//
// It is recommended to keep the implementation of these methods fast; therefore
// the only action here is to copy the packet to a queue where it can be
// retrieved later by the main thread to be displayed.
//--------------------------------------------------------------------------------
void onLiveDataAvailable(struct XsCallbackPlainC* cb, struct XsDevice* dev, const struct XsDataPacket* packet)
{
	(void)cb; (void)dev;
	PacketQueue_add(&packetQueue, packet);
}

//--------------------------------------------------------------------------------
int main(void)
{
	// Declare used variables
	XsDevice* foundDevice;
	XsControl* control;
	XsPortInfo* portInfo;
	XsPortInfo* foundPort;
	XsDeviceId* foundDeviceId;

	// Make sure to call the proper xxx_INIT macros to properly initialize structs
	XsPortInfoArray portInfoArray = XSPORTINFOARRAY_INITIALIZER;
	XsCallbackPlainC callbackHandler = XSCALLBACK_INITIALIZER;
	XsString deviceIdString = XSSTRING_INITIALIZER;
	XsString portNameString = XSSTRING_INITIALIZER;
	XsQuaternion quaternion = XSQUATERNION_INITIALIZER;
	XsEuler euler = XSEULER_INITIALIZER;
	XsOutputConfigurationArray mtmk4_configArray = XSOUTPUTCONFIGURATIONARRAY_INITIALIZER;
	XsDeviceMode mtix_deviceMode = XSDEVICEMODE_INITIALIZER;
	XsOutputConfiguration mtmk4_config = XSOUTPUTCONFIGURATION_INITIALIZER;

	XsSize portInfoIdx;

	int deviceFound;
	int configSucceeded;

	// Initialize global packet queue
	PacketQueue_init(&packetQueue);

	// Create XsControl object
	printf("Creating XsControl object...\n");

	control = XsControl_construct();
	assert(control != 0);

	// Scan for connected devices
	printf("Scanning for devices...\n");
	(void)XsScanner_scanPorts(&portInfoArray, XBR_Invalid, 100, TRUE, TRUE);

	// Find an MTi / MTx / MTmk4 device
	foundPort = 0;
	foundDeviceId = 0;
	portInfo = portInfoArray.m_data;
	deviceFound = FALSE;
	for (portInfoIdx = 0; portInfoIdx < portInfoArray.m_size; ++portInfoIdx)
	{
		foundPort = &portInfo[portInfoIdx];
		foundDeviceId = &foundPort->m_deviceId;
		if (XsDeviceId_isMt9c(foundDeviceId) || XsDeviceId_isLegacyMtig(foundDeviceId) || XsDeviceId_isMtMk4(foundDeviceId) || XsDeviceId_isFmt_X000(foundDeviceId))
		{
			deviceFound = TRUE;
			break;
		}
	}

	if (deviceFound)
	{
		XsDeviceId_toString(foundDeviceId, &deviceIdString);
		printf("Found a device with id: %s @ port: %s, baudrate: %d\n", deviceIdString.m_data, foundPort->m_portName, foundPort->m_baudrate);

		// Open the port with the detected device
		printf("Opening port...\n");
		XsString_assignCharArray(&portNameString, foundPort->m_portName);
		if (XsControl_openPort(control, &portNameString, foundPort->m_baudrate, 0, TRUE))
		{
			// Get the device object
			foundDevice = XsControl_device(control, foundDeviceId);
			assert(foundDevice != 0);

			// Print information about detected MTi / MTx / MTmk4 device
			printf("Device: %s openend.\n", XsDevice_productCode(foundDevice, &deviceIdString)->m_data);

			// Attach callback handler to device
			callbackHandler.m_onLiveDataAvailable = onLiveDataAvailable;
			XsDevice_addCallbackHandler(foundDevice, &callbackHandler, TRUE);

			// Put the device in configuration mode
			printf("Putting device into configuration mode...\n");
			configSucceeded = FALSE;
			if (XsDevice_gotoConfig(foundDevice)) // Put the device into configuration mode before configuring the device
			{
				// Configure the device. Note the differences between MTix and MTmk4
				printf("Configuring the device...\n");
				if (XsDeviceId_isMt9c(foundDeviceId) || XsDeviceId_isLegacyMtig(foundDeviceId))
				{
					memset(&mtix_deviceMode, 0, sizeof(XsDeviceMode));
					mtix_deviceMode.m_outputMode = XOM_Orientation; // output orientation data
					mtix_deviceMode.m_outputSettings = XOS_OrientationMode_Quaternion; // output orientation data as quaternion
					XsDeviceMode_setUpdateRate(&mtix_deviceMode, 100);// make a device mode with update rate: 100 Hz

					// set the device configuration
					if (XsDevice_setDeviceMode(foundDevice, &mtix_deviceMode))
					{
						configSucceeded = TRUE;
					}
				}
				else if (XsDeviceId_isMtMk4(foundDeviceId) || XsDeviceId_isFmt_X000(foundDeviceId))
				{
					mtmk4_config.m_dataIdentifier = XDI_Quaternion;
					mtmk4_config.m_frequency = XDI_MAX_FREQUENCY;
					XsOutputConfigurationArray_construct(&mtmk4_configArray, 1, &mtmk4_config);

					if (XsDevice_setOutputConfiguration(foundDevice, &mtmk4_configArray))
					{
						configSucceeded = TRUE;
					}
				}
				else
				{
					printf("Unknown device while configuring. Aborting.");
				}
			}
			else
			{
				printf("Could not put device into configuration mode. Aborting.\n");
			}

			if (configSucceeded)
			{
				// Put the device in measurement mode
				printf("Putting device into measurement mode...\n");
				if (XsDevice_gotoMeasurement(foundDevice))
				{
					printf("\nMain loop (press any key to quit)\n");
					printf("-------------------------------------------------------------------------------\n");
					while (!_kbhit())
					{
						// Retrieve a packet from queue
						XsDataPacket packet;
						XsDataPacket_construct(&packet);
						if (PacketQueue_remove(&packetQueue, &packet))
						{
							// Get the quaternion data
							XsDataPacket_orientationQuaternion(&packet, &quaternion, XDI_CoordSysEnu);

							// Convert packet to euler
							XsDataPacket_orientationEuler(&packet, &euler, XDI_CoordSysEnu);

							printf("\rq0:%5.2f,q1:%5.2f,q2:%5.2f,q3:%5.2f,roll:%7.2f,pitch:%7.2f,yaw:%7.2f",
								quaternion.m_w, quaternion.m_x, quaternion.m_y, quaternion.m_z,
								euler.m_roll, euler.m_pitch, euler.m_yaw
							);
							fflush(stdout);
						}
						XsDataPacket_destruct(&packet);
						XsTime_msleep(0);
					}
					_getch();
					printf("\n-------------------------------------------------------------------------------\n");
				}
				else
				{
					printf("Could not put device into measurement mode. Aborting.\n");
				}
			}
			else
			{
				printf("Could not configure device. Aborting.\n");
			}

			// Close port
			printf("Closing port...\n");
			XsControl_closePort(control, &portNameString);
		}
		else
		{
			printf("Could not open port. Aborting.\n");
		}
	}
	else
	{
		printf("No MTi / MTx device found. Aborting.\n");
	}

	// Free XsControl object
	printf("Freeing XsControl object...\n");
	XsControl_destruct(control);

	// free global packet queue
	PacketQueue_destruct(&packetQueue);

	printf("Successful exit.\n");
	printf("Press a key to continue.\n");
	(void)_getch();

	return 0;
}

//--------------------------------------------------------------------------------
