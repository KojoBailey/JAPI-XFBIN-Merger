#pragma once

#define EXPORT extern "C" __declspec(dllexport)

#include <JojoAPI.h>

EXPORT ModMeta __stdcall GetModInfo();
EXPORT void __stdcall ModInit();