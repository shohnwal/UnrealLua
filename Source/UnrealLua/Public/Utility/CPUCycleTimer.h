#pragma once
#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

struct UNREALLUA_API FCPUCycleTimer
{
	FCPUCycleTimer(const FString& msg)
	{
		message = msg;
		startCycles = FPlatformTime::Cycles64(); 
	}
	~FCPUCycleTimer()
	{
		
		uint64 endcycles = FPlatformTime::Cycles64();
		double startTime = FPlatformTime::ToMilliseconds64(startCycles); 
		double endTime = FPlatformTime::ToMilliseconds64(endcycles);
		UE_LOG(LogTemp, Log, TEXT("%s : code executed in %f ms and %llu cycles"), *message, endTime - startTime, endcycles - startCycles);		
	}
	FString message;
	uint64 startCycles = 0;
};
