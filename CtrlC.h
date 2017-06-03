#pragma once

// Set a Ctrl-C handler. If an empty function is passed (i.e. `std::function()`)
// then the default Ctrl-C behaviour is restored. If you just want it to do nothing
// pass a function that does nothing (i.e. `[] {}`).
//
// Returns true if the function was successfully set.

typedef void (*CtrlCCallback)();

bool SetCtrlCHandler(CtrlCCallback callback);
