#include <windows.h>
#include <string>
#include <ctime>
#include <vector>

// 引入绘图库
#pragma comment(lib, "Msimg32.lib") 

// ===========================================================================
// 🎛️🎛️🎛️ 老板最终定价参数 🎛️🎛️🎛️
// ===========================================================================

// 1. 【爆率设置】(老板指定：10%)
// 总中奖率：10% (90% 的时候通杀，10% 给点甜头)
const int TOTAL_WIN_RATE = 10;

// 大奖占比：1% (为了不亏钱，大奖必须卡得死死的！)
// 也就是说，在那 10% 的中奖里，只有 1% 是 777，剩下 99% 都是烂水果
const int BIG_WIN_RATIO = 1;

// 2. 【老虎机背景大小】
const int BG_FINAL_W = 800;
const int BG_FINAL_H = 600;

// 3. 【水果图标大小】
const int ICON_W = 50;
const int ICON_H = 50;

// 4. 【对齐位置】 (您的黄金坐标，绝对没动！)
const int SLOT_Y = 255;
const int SLOT_1_X = 155;
const int SLOT_2_X = 303;
const int SLOT_3_X = 445;

// ===========================================================================

// 全局变量
int g_num1 = 0; int g_num2 = 0; int g_num3 = 0;
bool g_isSpinning = false; int g_spinCount = 0;

const int IMAGE_COUNT = 4; // 0=苹果, 1=柠檬, 2=铃铛, 3=大奖777
HBITMAP g_hImages[IMAGE_COUNT];
HBITMAP g_hBg = NULL;

#define BUTTON_START_ID 1001
HWND hBtnStart;

// ---------------------------------------------------------
// 👑 核心逻辑：10% 胜率 + 柠檬平权 👑
// ---------------------------------------------------------
void UpdateSlots() {
    // 1. 扔骰子 (0-99)
    int luck = rand() % 100;

    // ★★★ 这里改成 10 了！★★★
    // 只有 luck < 10 的时候才让赢
    if (luck < TOTAL_WIN_RATE) {

        // 2. 就算赢了，看看给不给大奖？
        int bigLuck = rand() % 100;

        // 只有 1% 的机会给大奖 (保护老板钱包)
        if (bigLuck < BIG_WIN_RATIO) {
            // ---> 奇迹：发出 777 大奖 (3号图)
            g_num1 = 3; g_num2 = 3; g_num3 = 3;
        }
        else {
            // ---> 绝大多数情况：给小奖 (苹果 vs 柠檬 五五开)
            int normalIcon = rand() % 2;
            g_num1 = normalIcon; g_num2 = normalIcon; g_num3 = normalIcon;
        }
    }
    else {
        // 【通杀模式 (输)】
        // 确保三个数不完全一样
        do {
            g_num1 = rand() % IMAGE_COUNT;
            g_num2 = rand() % IMAGE_COUNT;
            g_num3 = rand() % IMAGE_COUNT;
        } while (g_num1 == g_num2 && g_num2 == g_num3);
    }
}

// 加载图片
void LoadMyImages(HWND hWnd) {
    for (int i = 0; i < IMAGE_COUNT; i++) {
        std::wstring fileName = std::to_wstring(i) + L".bmp";
        g_hImages[i] = (HBITMAP)LoadImage(NULL, fileName.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (g_hImages[i] == NULL) MessageBox(hWnd, (L"加载失败: " + fileName).c_str(), L"缺图", MB_ICONERROR);
    }
    g_hBg = (HBITMAP)LoadImage(NULL, L"老虎机.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (g_hBg == NULL) MessageBox(hWnd, L"找不到【老虎机.bmp】", L"缺背景", MB_ICONERROR);
}

// 窗口处理
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE:
        hBtnStart = CreateWindow(L"BUTTON", L"🎰 启动引擎 🎰", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)BUTTON_START_ID, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
        SendMessage(hBtnStart, WM_SETFONT, (WPARAM)CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑"), TRUE);
        srand((unsigned int)time(NULL));
        LoadMyImages(hWnd);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rcClient; GetClientRect(hWnd, &rcClient);
        int winW = rcClient.right - rcClient.left;
        int winH = rcClient.bottom - rcClient.top;

        // 白色背景
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rcClient, hBrush);
        DeleteObject(hBrush);

        HDC hdcMem = CreateCompatibleDC(hdc);
        int bgX = (winW - BG_FINAL_W) / 2;
        int bgY = (winH - BG_FINAL_H) / 2;

        // 画老虎机
        if (g_hBg != NULL) {
            BITMAP bgInfo; GetObject(g_hBg, sizeof(BITMAP), &bgInfo);
            SelectObject(hdcMem, g_hBg);
            StretchBlt(hdc, bgX, bgY, BG_FINAL_W, BG_FINAL_H, hdcMem, 0, 0, bgInfo.bmWidth, bgInfo.bmHeight, SRCCOPY);
        }

        // 画水果
        int currentIndices[3] = { g_num1, g_num2, g_num3 };
        int xOffsets[3] = { SLOT_1_X, SLOT_2_X, SLOT_3_X };

        for (int i = 0; i < 3; i++) {
            HBITMAP hBmpToDraw = g_hImages[currentIndices[i]];
            if (hBmpToDraw != NULL) {
                BITMAP itemInfo; GetObject(hBmpToDraw, sizeof(BITMAP), &itemInfo);
                SelectObject(hdcMem, hBmpToDraw);
                int finalX = bgX + xOffsets[i];
                int finalY = bgY + SLOT_Y;

                // 透明绘制
                TransparentBlt(hdc, finalX, finalY, ICON_W, ICON_H, hdcMem, 0, 0, itemInfo.bmWidth, itemInfo.bmHeight, RGB(255, 0, 255));
            }
        }
        MoveWindow(hBtnStart, (winW - 200) / 2, bgY + BG_FINAL_H + 10, 200, 50, TRUE);
        DeleteDC(hdcMem);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_COMMAND:
        if (LOWORD(wParam) == BUTTON_START_ID) {
            if (g_isSpinning) break;
            g_isSpinning = true; g_spinCount = 0; SetTimer(hWnd, 1, 50, NULL);
        }
        break;

    case WM_TIMER:
        if (g_spinCount < 30) {
            UpdateSlots(); g_spinCount++; InvalidateRect(hWnd, NULL, TRUE);
        }
        else {
            KillTimer(hWnd, 1); g_isSpinning = false;

            // 判定结果
            if (g_num1 == 3 && g_num2 == 3 && g_num3 == 3) {
                MessageBeep(MB_ICONASTERISK);
                MessageBox(hWnd, L"💰💰💰 居然中了大奖！老板心好痛！💰💰💰", L"奇迹", MB_OK);
            }
            else if (g_num1 == g_num2 && g_num2 == g_num3) {
                MessageBox(hWnd, L"🎉 恭喜！赢了点小钱！🎉", L"恭喜", MB_OK);
            }
        }
        break;

    case WM_DESTROY:
        if (g_hBg) DeleteObject(g_hBg);
        for (int i = 0; i < IMAGE_COUNT; i++) if (g_hImages[i]) DeleteObject(g_hImages[i]);
        PostQuitMessage(0);
        break;

    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.lpszClassName = L"SlotMachineBoss10Percent";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"SlotMachineBoss10Percent", L"我的C++老虎机 (10% 黄金胜率版)", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 1024, 768, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;
    ShowWindow(hWnd, nCmdShow); UpdateWindow(hWnd);
    MSG msg; while (GetMessage(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}