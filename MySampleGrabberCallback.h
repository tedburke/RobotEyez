//
// MySampleGrabberCallback.h - MySampleGrabberCallback header file
// Written by Ted Burke - last modified 23-11-2011
//

// DirectShow header files
#include <dshow.h>
#include <streams.h>

#import "qedit.dll" raw_interfaces_only named_guids

// Create DirectShow callback
class MySampleGrabberCallback : DexterLib::ISampleGrabberCB
{
private:
	//! COM reference count
	ULONG _ref_count;

public:
	//! Return a ptr to a different COM interface to this object
	HRESULT APIENTRY QueryInterface( REFIID iid, void** ppvObject )
	{
		// Match the interface and return the proper pointer
		if ( iid == IID_IUnknown) {
			*ppvObject = dynamic_cast<IUnknown*>( this );
		} else if ( iid == DexterLib::IID_ISampleGrabberCB ) {
			*ppvObject = dynamic_cast<ISampleGrabberCB*>( this );
		} else {
			// It didn't match an interface
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}

		// Increment refcount on the pointer we're about to return
		this->AddRef();
		// Return success
		return S_OK;
	}

	//! Increment ref count
	ULONG APIENTRY AddRef()
	{
		return ( ++ _ref_count );
	}

	//! Decrement ref count
	ULONG APIENTRY Release()
	{
		return ( -- _ref_count );
	}
	
	HRESULT APIENTRY BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen);
	HRESULT APIENTRY SampleCB(double SampleTime, DexterLib::IMediaSample *pSample);
};
