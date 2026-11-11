#pragma once
#include <windows.h>
#include "PIFilter.h"
#include "tint_core.hpp"
namespace tint {
INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, Parameters& values);
}
