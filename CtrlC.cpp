#include "CtrlC.h"

static std::function<void()> userCallback;

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


bool SetCtrlCHandler(std::function<void ()> callback)
{
	userCallback = callback;
	return SetConsoleCtrlHandler(CtrlCHandler, callback ? TRUE : FALSE) != 0;
}

#elif defined(__unix)

#include <signal.h>
#include <unistd.h>

static void CtrlCHandler(int s)
{

}

bool SetCtrlCHandler(std::functional<void()> callback)
{
	userCallback = callback;

	if (callback)
	{
		sigaction sigIntHandler;

		sigIntHandler.sa_handler = CtrlCHandler;
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;

		return sigaction(SIGINT, &sigIntHandler, nullptr) == 0;
	}
	else
	{
		// ???
	}
}


#else
#error Ctrl-C support not written for this platform yet.
#endif