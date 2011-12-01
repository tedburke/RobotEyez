//
// FrameTransformFilter.h - FrameTransformFilter header file
// Written by Ted Burke - last modified 30-11-2011
//
// Copyright Ted Burke, 2011, All rights reserved.
//
// Website: http://batchloaf.wordpress.com
//

#ifndef FRAMETRANSFORMFILTER_H
#define FRAMETRANSFORMFILTER_H

#include <dshow.h>
#include <streams.h>

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

// I generated the following GUID for this filter using the
// online GUID generator at http://www.guidgen.com/
// {d6ece2e3-72aa-4157-b489-52c3fd693ce9}
DEFINE_GUID(CLSID_FrameTransformFilter, 
0xd6ece2e3, 0x72aa, 0x4157, 0xb4, 0x89, 0x52, 0xc3, 0xfd, 0x69, 0x3c, 0xe9);

// My frame transforming filter class
class FrameTransformFilter : public CTransformFilter
{
public:
	// Constructor
	FrameTransformFilter();
	
	// Provide a function to allow a PGM file copy of the
	// next frame to be requested
	void saveNextFrameToFile(char *filename);
	void setCommand(char *command);
	int filesSaved();
	
	// Methods required for filters derived from CTransformFilter
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
	HRESULT Transform(IMediaSample *pSource, IMediaSample *pDest);
	
private:
	int save_frame_to_file;	// flag to request saving next frame to PGM file
	char filename[200];	// filename to use when saving a capture frame to file
	int files_saved;	// counter for number of frames saved to PGM files
	int run_command;	// flag to execute program after each file capture
	char command[200];	// command to run after each image file is saved
};

#endif // FRAMETRANSFORMFILTER_H
