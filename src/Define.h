#pragma once

#ifdef _WIN32
#ifdef NX_PLUGIN_CLIP_EXPORTS
#define NX_PLUGIN_CLIP __declspec(dllexport)
#else
#define NX_PLUGIN_CLIP __declspec(dllimport)
#endif
#else
#define NX_PLUGIN_CLIP
#endif

#define DELETE_PTR(ptr) { if (ptr) delete ptr; ptr = nullptr; }
#define DELETE_ARRAY_PTR(ptr) { if (ptr) delete[] ptr; ptr = nullptr; }
