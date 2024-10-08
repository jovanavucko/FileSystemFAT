#pragma once

#include <Windows.h>

#define signalA(x) ReleaseSemaphore(x,1,NULL)
#define waitA(x) WaitForSingleObject(x,INFINITE)