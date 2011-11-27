//
// FrameTransformFilter.h - FrameTransformFilter header file
// Written by Ted Burke - last modified 23-11-2011
//

#ifndef FRAMETRANSFORMFILTER_H
#define FRAMETRANSFORMFILTER_H

#include <dshow.h>
#include <streams.h>

// I generated the following GUID for this filter using the
// online GUID generator at http://www.guidgen.com/
// {d6ece2e3-72aa-4157-b489-52c3fd693ce9}
DEFINE_GUID(CLSID_FrameTransformFilter, 
0xd6ece2e3, 0x72aa, 0x4157, 0xb4, 0x89, 0x52, 0xc3, 0xfd, 0x69, 0x3c, 0xe9);

// Frame transformer filter class
class FrameTransformFilter : public CTransformFilter
{
public:
	// Constructor
	FrameTransformFilter();
	
	// Provide a function to allow a PGM file copy of the
	// next frame to be requested
	void saveNextFrameToPGMFile(char *filename);
	int filesSaved();
	
	// Methods required for filters derived from CTransformFilter
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
	HRESULT Transform(IMediaSample *pSource, IMediaSample *pDest);
	
private:
	// Flag to save next frame to PGM file
	char filename[200];
	int save_to_PGM;
	int files_saved;
};

#endif // FRAMETRANSFORMFILTER_H
