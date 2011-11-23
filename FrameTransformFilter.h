//
// FrameTransformFilter.h - FrameTransformFilter header file
// Written by Ted Burke - last modified 23-11-2011
//

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
	FrameTransformFilter::FrameTransformFilter();
	
	// Methods required for filters derived from CTransformFilter
	HRESULT FrameTransformFilter::CheckInputType(
		const CMediaType *mtIn);
	HRESULT FrameTransformFilter::GetMediaType(
		int iPosition, CMediaType *pMediaType);
	HRESULT FrameTransformFilter::CheckTransform(
		const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT FrameTransformFilter::DecideBufferSize(
		IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
	HRESULT FrameTransformFilter::Transform(
		IMediaSample *pSource, IMediaSample *pDest);
};
