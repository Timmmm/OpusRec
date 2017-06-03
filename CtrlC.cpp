#include "CtrlC.h"

static CtrlCCallback userCallback = nullptr;

#if defined(_WIN32)

#include <windows.h>

WINBOOL WINAPI CtrlCHandler(DWORD fdwCtrlType)
{
	switch(fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		if (userCallback)
			userCallback();
		return TRUE;
	default:
		break;
	}
	// Return false to indicate that we aren't handling the signal.
	return FALSE;
}


bool SetCtrlCHandler(CtrlCCallback callback)
{
	userCallback = callback;
	return SetConsoleCtrlHandler(CtrlCHandler, callback ? TRUE : FALSE) != 0;
}

#elif defined(__unix)

#include <signal.h>
#include <unistd.h>

static void CtrlCHandler(int s)
{
	if (userCallback)
		userCallback();
}

bool SetCtrlCHandler(CtrlCCallback callback)
{
	userCallback = callback;

	if (callback)
	{
		struct sigaction sigIntHandler;

		sigIntHandler.sa_handler = CtrlCHandler;
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;

		return sigaction(SIGINT, &sigIntHandler, nullptr) == 0;
	}
	else
	{
		struct sigaction sigIntHandler;

		sigIntHandler.sa_handler = SIG_DFL; // Restore default.
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;

		return sigaction(SIGINT, &sigIntHandler, nullptr) == 0;
	}
	return false;
}


#else
#error Ctrl-C support not written for this platform yet.
#endif
