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

#ifndef PACKET_XS_DEVICECALLBACKHANDLER_H
#define PACKET_XS_DEVICECALLBACKHANDLER_H

#include <xsensdeviceapi.h>

namespace XsDemo
{

//--------------------------------------------------------------------------------
class XsDeviceCallbackHandler : public XsCallback
{
public:
	XsDeviceCallbackHandler(size_t maxBufferSize = 10) : m_maxNumberOfPacketsInBuffer(maxBufferSize), m_numberOfPacketsInBuffer(0) 
	
#ifdef _MSC_VER
	{InitializeCriticalSection(&m_CriticalSection);}	  	 
	virtual ~XsDeviceCallbackHandler() throw() {DeleteCriticalSection(&m_CriticalSection);}	
#else
	{
	  //create mutex attribute variable
	  pthread_mutexattr_t mAttr;

	  // setup recursive mutex for mutex attribute
	  pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE_NP);

	  // Use the mutex attribute to create the mutex
	  pthread_mutex_init(&m_CriticalSection, &mAttr);

	  // Mutex attribute can be destroy after initializing the mutex variable
	  pthread_mutexattr_destroy(&mAttr);
	  
	}	  	 
	virtual ~XsDeviceCallbackHandler() throw() {pthread_mutex_destroy(&m_CriticalSection);}	
#endif

	size_t numberOfPacketsInBuffer() const {Locker lock(*this); return m_numberOfPacketsInBuffer;}
	size_t maxNumberOfPacketsInBuffer() const {return m_maxNumberOfPacketsInBuffer;}
	XsDataPacket popOldestPacket() {Locker lock(*this); XsDataPacket oldestPacket(m_packetBuffer.front()); m_packetBuffer.pop_front(); --m_numberOfPacketsInBuffer; return oldestPacket;}	

protected:
	virtual void onLiveDataAvailable(XsDevice*, const XsDataPacket* packet)
	{	
		Locker lock(*this);
		assert(packet != 0);	
		while (m_numberOfPacketsInBuffer >= m_maxNumberOfPacketsInBuffer)
		{			
			(void)popOldestPacket();			
		}
		m_packetBuffer.push_back(*packet);
		++m_numberOfPacketsInBuffer;
		assert(m_numberOfPacketsInBuffer <= m_maxNumberOfPacketsInBuffer);
	}
private:	
#ifdef _MSC_VER
	mutable CRITICAL_SECTION m_CriticalSection;
#else
	mutable pthread_mutex_t m_CriticalSection;
#endif
	struct Locker
	{
#ifdef _MSC_VER
		Locker(XsDeviceCallbackHandler const & self) : m_self(self) {EnterCriticalSection(&m_self.m_CriticalSection);}
		~Locker() throw() {LeaveCriticalSection(&m_self.m_CriticalSection);}
#else
		Locker(XsDeviceCallbackHandler const & self) : m_self(self) {pthread_mutex_lock(&m_self.m_CriticalSection);}
		~Locker() throw() {pthread_mutex_unlock(&m_self.m_CriticalSection);}
#endif
		XsDeviceCallbackHandler const & m_self;
	};

	size_t m_maxNumberOfPacketsInBuffer;
	size_t m_numberOfPacketsInBuffer;
	std::list<XsDataPacket> m_packetBuffer;
};

//--------------------------------------------------------------------------------

}

#endif

