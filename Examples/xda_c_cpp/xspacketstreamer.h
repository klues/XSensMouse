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

#ifndef PACKET_STREAMER_H
#define PACKET_STREAMER_H

#include <ostream>
#include <xsensdeviceapi.h>

namespace XsDemo
{

//--------------------------------------------------------------------------------
class IXsPacketStreamer
{
public:
	virtual ~IXsPacketStreamer() throw() {}
	virtual int stream(std::ostream& out, XsDataPacket const & packet) const = 0;
};

//--------------------------------------------------------------------------------
class XsPacketEulerStreamer : public virtual IXsPacketStreamer
{
public:
	virtual int stream(std::ostream& out, XsDataPacket const & packet) const
	{
		if (!packet.containsOrientation())
			return 0;

		XsEuler euler = packet.orientationEuler();
		out << "Orientation [Euler] -- "
			<< std::showpos
			<< "Roll: " << std::setw(8) << std::fixed << std::setprecision(3) << euler.roll()
			<< ", Pitch: " << std::setw(8) << std::fixed << std::setprecision(3) << euler.pitch()
			<< ", Yaw: " << std::setw(8) << std::fixed << std::setprecision(3) << euler.yaw()
			<< std::noshowpos
		;

		return 1; // number of lines
	}
};

//--------------------------------------------------------------------------------
class XsPacketQuaternionStreamer : public virtual IXsPacketStreamer
{
public:
	virtual int stream(std::ostream& out, XsDataPacket const & packet) const
	{
		if (!packet.containsOrientation())
			return 0;

		XsQuaternion quaternation = packet.orientationQuaternion();
		out << "Orientation [Quaternion] -- "
			<< std::showpos
			<< "q0: " << std::setw(7) << std::fixed << std::setprecision(3) << quaternation.w()
			<< ", q1: " << std::setw(7) << std::fixed << std::setprecision(3) << quaternation.x()
			<< ", q2: " << std::setw(7) << std::fixed << std::setprecision(3) << quaternation.y()
			<< ", q3: " << std::setw(7) << std::fixed << std::setprecision(3) << quaternation.z()
			<< std::noshowpos
		;

		return 1; // number of lines
	}
};

//--------------------------------------------------------------------------------
class XsPacketMatrixStreamer : public virtual IXsPacketStreamer
{
public:
	virtual int stream(std::ostream& out, XsDataPacket const & packet) const
	{
		if (!packet.containsOrientation())
			return 0;

		XsMatrix3x3 matrix = packet.orientationMatrix();
		out << "Orientation [Matrix] -- "
			<< std::showpos
			<< std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(0, 0)
			<< "  " << std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(0, 1)
			<< "  " << std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(0, 2) << std::endl

			<< "                        "
			<< std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(1, 0)
			<< "  " << std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(1, 1)
			<< "  " << std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(1, 2) << std::endl

			<< "                        "
			<< std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(2, 0)
			<< "  " << std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(2, 1)
			<< "  " << std::setw(7) << std::fixed << std::setprecision(3) << matrix.value(2, 2)
			<< std::noshowpos
		;

		return 3; // number of lines
	}
};

//--------------------------------------------------------------------------------
class XsPacketSdiStreamer : public virtual IXsPacketStreamer
{
public:
	virtual int stream(std::ostream& out, XsDataPacket const & packet) const
	{
		if (!packet.containsSdiData())
			return 0;

		XsSdiData sdiData = packet.sdiData();
		out << "Orientation [SDI] -- "
			<< std::showpos
			<< "Orientation incr. : " << std::setw(7) << std::fixed << std::setprecision(3) << sdiData.orientationIncrement()[0]
			<< ", " << std::setw(7) << std::fixed << std::setprecision(3) << sdiData.orientationIncrement()[1]
			<< ", " << std::setw(7) << std::fixed << std::setprecision(3) << sdiData.orientationIncrement()[2]
			<< ", " << std::setw(7) << std::fixed << std::setprecision(3) << sdiData.orientationIncrement()[3] << std::endl

			<< "                     "
			<< "Velocity incr. :    " << std::setw(7) << std::fixed << std::setprecision(3) << sdiData.velocityIncrement()[0]
			<< ", " << std::setw(7) << std::fixed << std::setprecision(3) << sdiData.velocityIncrement()[1]
			<< ", " << std::setw(7) << std::fixed << std::setprecision(3) << sdiData.velocityIncrement()[2]
			<< std:: noshowpos
		;

		return 2; // number of lines
	}
};

//--------------------------------------------------------------------------------
class XsPacketRawStreamer : public virtual IXsPacketStreamer
{
public:
	virtual int stream(std::ostream& out, XsDataPacket const & packet) const
	{
		if (!packet.containsRawData())
			return 0;

		XsScrData rawData = packet.rawData();

		XsUShortVector&	acc = rawData.m_acc;
		XsUShortVector&	gyr = rawData.m_gyr;
		XsUShortVector&	mag = rawData.m_mag;
		uint16_t tmp = rawData.m_temp;

		out << std::showpos
			<< "Raw Data -- " << "acc: [" << std::setw(5) << acc.at(0) << ", " << std::setw(5) << acc.at(1) << ", " << std::setw(5) << acc.at(2) << "]        " << std::endl
			<< "            " << "gyr: [" << std::setw(5) << gyr.at(0) << ", " << std::setw(5) << gyr.at(1) << ", " << std::setw(5) << gyr.at(2) << "]        " << std::endl
			<< "            " << "mag: [" << std::setw(5) << mag.at(0) << ", " << std::setw(5) << mag.at(1) << ", " << std::setw(5) << mag.at(2) << "]        " << std::endl
			<< "            " << "tmp: ["
			<< std::noshowpos
		;
		out << tmp << "]        ";

		return 4; // number of lines
	}
};

//--------------------------------------------------------------------------------

}

#endif
