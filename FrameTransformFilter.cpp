//
// FrameTransformFilter.cpp - FrameTransformFilter class
// Written by Ted Burke - last modified 23-11-2011
//
// Most of the code below is copied from the MSDN
// "Writing Transform Filters" tutorial.
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd391015(v=vs.85).aspx
//

// DirectShow header files
#include <dshow.h>
#include <streams.h>
#include <Aviriff.h>

#include <initguid.h>
#include "FrameTransformFilter.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

FrameTransformFilter::FrameTransformFilter()
  : CTransformFilter(NAME("My Frame Transforming Filter"), 0, CLSID_FrameTransformFilter)
{ 
	// Initialize any private variables here
	save_to_PGM = 0;
	files_saved = 0;
}

void FrameTransformFilter::saveNextFrameToPGMFile(char *output_filename)
{
	// Remember filename and set flag to request dump
	// of next frame to PGM file
	strncpy(filename, output_filename, 200);
	save_to_PGM = 1;
}

int FrameTransformFilter::filesSaved()
{
	return files_saved;
}

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

HRESULT FrameTransformFilter::Transform(IMediaSample *pSource, IMediaSample *pDest)
{
    BYTE *pBufferIn, *pBufferOut;
    HRESULT hr;
	
    // Get pointers to the underlying buffers.
    if (FAILED(hr = pSource->GetPointer(&pBufferIn))) return hr;
    if (FAILED(hr = pDest->GetPointer(&pBufferOut))) return hr;
	
    // Process the data.
	//memcpy(pBufferOut, pBufferIn, pSource->GetSize());
	int val = 0, n = 0, N = pSource->GetSize();
	while (n < N)
	{
		// Modify current pixel
		val = (pBufferIn[n] + pBufferIn[n+1] + pBufferIn[n+2]) / 3;
		pBufferOut[n++] = val;
		pBufferOut[n++] = val;
		pBufferOut[n++] = val;
	}
	
	// Output greyscale image to PGM file if flag is set
	FILE *f;
	if (save_to_PGM)
	{
		if (f = fopen(filename, "w"))
		{
			// Write current frame to PGM file
			fprintf(f, "P2\n# Frame captured by RobotEyez\n%d %d\n%d\n",
					FRAME_HEIGHT, FRAME_WIDTH);
			n = 0;
			for (int y=0 ; y<FRAME_HEIGHT ; ++y)
			{
				for (int x=0 ; x<FRAME_WIDTH ; ++x)
				{
					fprintf(f, "%d ", pBufferOut[n]);
					n += 3;
				}
				fprintf(f, "\n");
			}
			fclose(f);
			
			// Increment frame counter and reset flag
			files_saved++;
			save_to_PGM = 0;
		}
		else
		{
			fprintf(stderr, "Error opening file %s\n", filename);
		}
	}

    pDest->SetActualDataLength(pSource->GetActualDataLength());
    pDest->SetSyncPoint(TRUE);
    return S_OK;
}
