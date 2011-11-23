//
// MySampleGrabberCallback.cpp - MySampleGrabberCallback class
// Written by Ted Burke - last modified 23-11-2011
//

#include "MySampleGrabberCallback.h"

HRESULT APIENTRY MySampleGrabberCallback::BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen)
{
	for (int n=0 ; n<BufferLen ; ++n)
	{
		pBuffer[n] = 255 - pBuffer[n];
	}
	fprintf(stderr, "b");
	return S_OK;
}

HRESULT APIENTRY MySampleGrabberCallback::SampleCB(double SampleTime, DexterLib::IMediaSample *pSample)
{
	fprintf(stderr, "s");
	return S_OK;
}
