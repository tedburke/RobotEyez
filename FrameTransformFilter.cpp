//
// FrameTransformFilter.cpp - FrameTransformFilter class
// Written by Ted Burke - last modified 30-11-2011
//
// Copyright Ted Burke, 2011, All rights reserved.
//
// Website: http://batchloaf.wordpress.com
//
// Some of the code below is adapted from the MSDN
// "Writing Transform Filters" tutorial.
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd391015(v=vs.85).aspx
//

// DirectShow header files
#include <dshow.h>
#include <streams.h>
#include <initguid.h>

#include "FrameTransformFilter.h"

// Used for various strings - filenames, command line args, etc
// (also defined in RobotEyez.cpp)
#define STRING_LENGTH 200

// Function prototype
int write_pgm_file(char *, unsigned char *, int, int);
int write_bmp_file(char *, unsigned char *, int, int);

char text_buffer[2*STRING_LENGTH];

FrameTransformFilter::FrameTransformFilter()
  : CTransformFilter(NAME("My Frame Transforming Filter"), 0, CLSID_FrameTransformFilter)
{
	// Initialize any private variables here
	save_frame_to_file = 0;
	files_saved = 0;
	run_command = 0;
}

//
// This function is used during DirectShow graph building
// to limit the type of input connection that the filter
// will accept.
// Here, the connection must be 640x480 24-bit RGB.
//
HRESULT FrameTransformFilter::CheckInputType(const CMediaType *mtIn)
{
	VIDEOINFOHEADER *pVih = 
		reinterpret_cast<VIDEOINFOHEADER*>(mtIn->pbFormat);
	
	if ((mtIn->majortype != MEDIATYPE_Video) ||
		(mtIn->subtype != MEDIASUBTYPE_RGB24) ||
		(mtIn->formattype != FORMAT_VideoInfo) || 
		(mtIn->cbFormat < sizeof(VIDEOINFOHEADER)) ||
		(pVih->bmiHeader.biPlanes != 1) ||
		(pVih->bmiHeader.biWidth != FRAME_WIDTH) ||
		(pVih->bmiHeader.biHeight != FRAME_HEIGHT) ||
		(pVih->bmiHeader.biBitCount != 24) ||
		(pVih->bmiHeader.biCompression != BI_RGB))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	
	return S_OK;
}

//
// This function is called to find out what this filters
// preferred output format is. Here, the output type is
// specified at 640x480 24-bit RGB.
//
HRESULT FrameTransformFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	HRESULT hr;

	ASSERT(m_pInput->IsConnected());

	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	if (FAILED(hr = m_pInput->ConnectionMediaType(pMediaType))) return hr;

	ASSERT(pMediaType->formattype == FORMAT_VideoInfo);
	VIDEOINFOHEADER *pVih =
		reinterpret_cast<VIDEOINFOHEADER*>(pMediaType->pbFormat);
	pVih->bmiHeader.biCompression = BI_RGB;
	pVih->bmiHeader.biSizeImage = DIBSIZE(pVih->bmiHeader);
	pVih->bmiHeader.biPlanes = 1;
	pVih->bmiHeader.biBitCount = 24;
	pVih->bmiHeader.biCompression = BI_RGB;
	pVih->bmiHeader.biWidth = FRAME_WIDTH;
	pVih->bmiHeader.biHeight = FRAME_HEIGHT;

	return S_OK;
}

//
// This function is used to verify that the proposed
// connections into and out of the filter are acceptable
// before the capture graph is run.
//
HRESULT FrameTransformFilter::CheckTransform(
	const CMediaType *mtIn, const CMediaType *mtOut)
{
	// Check the major type.
	if ((mtOut->majortype != MEDIATYPE_Video) ||
		(mtOut->formattype != FORMAT_VideoInfo) || 
		(mtOut->cbFormat < sizeof(VIDEOINFOHEADER)))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	
	// Compare the bitmap information against the input type.
	ASSERT(mtIn->formattype == FORMAT_VideoInfo);
	BITMAPINFOHEADER *pBmiOut = HEADER(mtOut->pbFormat);
	BITMAPINFOHEADER *pBmiIn = HEADER(mtIn->pbFormat);
	if ((pBmiOut->biPlanes != 1) ||
		(pBmiOut->biBitCount != 24) ||
		(pBmiOut->biCompression != BI_RGB) ||
		(pBmiOut->biWidth != pBmiIn->biWidth) ||
		(pBmiOut->biHeight != pBmiIn->biHeight))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	
	// Compare source and target rectangles.
	RECT rcImg;
	SetRect(&rcImg, 0, 0, pBmiIn->biWidth, pBmiIn->biHeight);
	RECT *prcSrc = &((VIDEOINFOHEADER*)(mtIn->pbFormat))->rcSource;
	RECT *prcTarget = &((VIDEOINFOHEADER*)(mtOut->pbFormat))->rcTarget;
	if ((!IsRectEmpty(prcSrc) && !EqualRect(prcSrc, &rcImg)) ||
		(!IsRectEmpty(prcTarget) && !EqualRect(prcTarget, &rcImg)))
	{
		return VFW_E_INVALIDMEDIATYPE;
	}
	
	// Everything is good.
	return S_OK;
}

//
// Can't remember exactly what this function does.
// This is probably modified very little (if at all)
// from the original one in the MSDN tutorial.
//
HRESULT FrameTransformFilter::DecideBufferSize(
	IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	AM_MEDIA_TYPE mt;
	HRESULT hr = m_pOutput->ConnectionMediaType(&mt);
	if (FAILED(hr))
	{
		return hr;
	}
	
	ASSERT(mt.formattype == FORMAT_VideoInfo);
	BITMAPINFOHEADER *pbmi = HEADER(mt.pbFormat);
	pProp->cbBuffer = DIBSIZE(*pbmi) * 2; 
	if (pProp->cbAlign == 0) pProp->cbAlign = 1;
	if (pProp->cBuffers == 0) pProp->cBuffers = 1;
	
	// Release the format block.
	FreeMediaType(mt);
	
	// Set allocator properties.
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) return hr;
	
	// Even when it succeeds, check the actual result.
	if (pProp->cbBuffer > Actual.cbBuffer) return E_FAIL;
	
	return S_OK;
}

//
// This function is called to process the image data
// each time a new frame is received
//
HRESULT FrameTransformFilter::Transform(
	IMediaSample *pSource, IMediaSample *pDest)
{
	BYTE *pBufferIn, *pBufferOut;
	HRESULT hr;
	
	// Get pointers to the underlying buffers.
	if (FAILED(hr = pSource->GetPointer(&pBufferIn))) return hr;
	if (FAILED(hr = pDest->GetPointer(&pBufferOut))) return hr;

	// Just copy data from input buffer to output buffer
	memcpy(pBufferOut, pBufferIn, pSource->GetSize());
	
	pDest->SetActualDataLength(pSource->GetActualDataLength());
	pDest->SetSyncPoint(TRUE);
	
	// Output image to PGM or BMP file if flag is set
	if (save_frame_to_file)
	{		
		// Check if file exists already
		FILE *f;
		if (f = fopen(filename, "r"))
		{
			// File can be opened for reading, so it exists
			fclose(f);
			
			// Nasty hack to check if file is currently open
			// in another program. There had been a problem
			// without this because another program could be
			// in the middle of reading the file.
			while (rename(filename, filename) != 0);
		}
		
		if (strcmp(filename+(strlen(filename)-4), ".pgm") == 0)
		{
			// Write current frame to PGM file
			write_pgm_file(filename, pBufferOut,
							FRAME_WIDTH, FRAME_HEIGHT);
		}
		else if (strcmp(filename+(strlen(filename)-4), ".bmp") == 0)
		{
			// Write current frame to BMP file
			write_bmp_file(filename, pBufferIn,
							FRAME_WIDTH, FRAME_HEIGHT);
		}
		
		// Increment frame counter and reset flag
		files_saved++;
		save_frame_to_file = 0;
		
		// Execute frame processing program if user has
		// specified one
		sprintf(text_buffer, "%s %s", command, filename);
		if (run_command) system(text_buffer);
	}
	
	return S_OK;
}

//
// This function is used to request that the next frame
// captured be saved to a PGM file
//
void FrameTransformFilter::saveNextFrameToFile(char *output_filename)
{
	// Remember filename and set flag to request dump
	// of next frame to PGM file
	strncpy(filename, output_filename, STRING_LENGTH);
	save_frame_to_file = 1;
}

//
// This function returns the number of image files
// that have been saved since the filter was created
//
int FrameTransformFilter::filesSaved()
{
	return files_saved;
}

//
// This function is used to designate an external program
// which will be launched each time an image file is saved
//
void FrameTransformFilter::setCommand(char *command_string)
{
	// Set flag to run command after each file is saved
	// and remember the command to run
	strncpy(command, command_string, STRING_LENGTH);
	run_command = 1;
}

int write_pgm_file(char *filename, unsigned char *pBuf, int w, int h)
{
	int x, y, val, n;
	
	// Process the data.
	/*
	int val = 0, n = 0, N = pSource->GetSize();
	while (n < N)
	{
		// Modify current pixel
		val = (pBufferIn[n] + pBufferIn[n+1] + pBufferIn[n+2]) / 3;
		pBufferOut[n++] = val;
		pBufferOut[n++] = val;
		pBufferOut[n++] = val;
	}
	*/

	FILE *f;
	if (f = fopen(filename, "w"))
	{
		// Write current frame to PGM file
		fprintf(f, "P2\n# Frame captured by RobotEyez\n%d %d\n255\n",
				FRAME_WIDTH, FRAME_HEIGHT);
		for (int y=FRAME_HEIGHT-1 ; y>=0 ; --y)
		{
			for (int x=0 ; x<FRAME_WIDTH ; ++x)
			{
				n = 3*(y*FRAME_WIDTH + x);
				val = (pBuf[n] + pBuf[n+1] + pBuf[n+2]) / 3;
				fprintf(f, "%d ", val);
			}
			fprintf(f, "\n");
		}
		fclose(f);
	}
	
	return 0;
}

int write_bmp_file(char *filename, unsigned char *pBuf, int w, int h)
{
	int x, y;
	
	// Bitmap structures to be written to file
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	
	// Fill BITMAPFILEHEADER structure
	memcpy((char *)&bfh.bfType, "BM", 2);
	bfh.bfSize = sizeof(bfh) + sizeof(bih) + 3*h*w;
	bfh.bfReserved1 = 0;
	bfh.bfReserved2 = 0;
	bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
	
	// Fill BITMAPINFOHEADER structure
	bih.biSize = sizeof(bih);
	bih.biWidth = w;
	bih.biHeight = h;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB; // uncompressed 24-bit RGB
	bih.biSizeImage = 0; // can be zero for BI_RGB bitmaps
	bih.biXPelsPerMeter = 3780; // 96dpi equivalent
	bih.biYPelsPerMeter = 3780;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;
	
	// Open bitmap file (binary mode)
	FILE *f;
	f = fopen(filename, "wb");
	if (f == NULL) return 1;
	
	// Write bitmap file header
	fwrite(&bfh, 1, sizeof(bfh), f);
	fwrite(&bih, 1, sizeof(bih), f);
	
	// Write bitmap pixel data starting with the
	// bottom line of pixels, left hand side
	fwrite(pBuf, 1, 3*w*h, f);
	/*
	for (y=0 ; y<h ; ++y)
	{
		for (x=0 ; x<w ; ++x)
		{
			// Write pixel components in BGR order
			fputc(pBuf[3*(y*w+x)+2], f);
			fputc(pBuf[3*(y*w+x)+1], f);
			fputc(pBuf[3*(y*w+x)], f);
		}
	}
	*/
	
	// Close bitmap file
	fclose(f);
	
	return 0;
}
