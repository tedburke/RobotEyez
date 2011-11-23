RobotEyez.exe: RobotEyez.cpp FrameTransformFilter.cpp FrameTransformFilter.h
	cl RobotEyez.cpp FrameTransformFilter.cpp /I"C:\Program Files\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\baseclasses" /MD -link /LIBPATH:"C:\Program Files\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\baseclasses\Release" ole32.lib strmiids.lib oleaut32.lib strmbase.lib Winmm.lib
