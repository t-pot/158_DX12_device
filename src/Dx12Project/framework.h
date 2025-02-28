// header.h : 標準のシステム インクルード ファイルのインクルード ファイル、
// またはプロジェクト専用のインクルード ファイル
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーからほとんど使用されていない部分を除外する
// Windows ヘッダー ファイル
#include <windows.h>
#include <shellapi.h>
#include <winrt/base.h>					// C++/WinRT
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")
#include <d3dx12.h>
//#include <D3Dcompiler.h>
// #pragma comment(lib, "d3dcompiler.lib")
//#include <DirectXMath.h>
// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
