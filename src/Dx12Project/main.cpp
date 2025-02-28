// main.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "main.h"
#include "Application.h"
using namespace tpot;

#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;                       // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];         // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];   // メイン ウィンドウ クラス名

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 選択されたメニューの解析:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DX12PROJECT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DX12PROJECT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow, unsigned int width, unsigned int height)
{
    hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

    DWORD style = WS_OVERLAPPEDWINDOW;

    RECT rc = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&rc, style, FALSE);

    return CreateWindowW(szWindowClass, szTitle, style,
        CW_USEDEFAULT, 0, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
}

//
// 引数の解析
//
void ParseCommandLineArgs(ApplicationArgs &args, WCHAR* argv[], int argc, WCHAR* szTitle)
{
	for (int i = 1; i < argc; ++i)// 0は実行ファイル名が入るので飛ばす
    {
        if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
            _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
        {
            args.useWarpDevice = true;

            // ウィンドウタイトルも更新
            std::wstring title(szTitle);
            title += TEXT(" (WARP)");
            wcsncpy_s(szTitle, MAX_LOADSTRING, title.c_str(), _TRUNCATE);
        }
    }
}

//
//  関数: wWinMain()
//
//  目的: メイン関数
//
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DX12PROJECT, szWindowClass, MAX_LOADSTRING);

    unsigned int width = 1280;
    unsigned int height = 720;

    // アプリケーションクラスの作成
    // (やりやすさからコマンドライン引数をlpCmdLineでなくて取得し直す)
    ApplicationArgs args;
    args.幅 = width;
    args.高さ = height;
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    ParseCommandLineArgs(args, argv, argc, szTitle);

    Application app(args);

	// ウィンドウ クラスを登録
    MyRegisterClass(hInstance);

    // アプリケーション初期化の実行:
    HWND hWnd = InitInstance(hInstance, nCmdShow, width, height);
    if (!hWnd){return FALSE;}

    if (app.Initialize(hWnd) != 0) { return FALSE; };

    ShowWindow(hWnd, nCmdShow);

    // メイン メッセージ ループ:
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DX12PROJECT));
    [&app, hAccelTable]() {
        do {
            MSG msg = {};
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);

                    if (msg.message == WM_QUIT) { return; }
                }
            }

        } while (!app.Update());
    }();

    // 後片付け
    app.Finalize();

    return 0;
}


