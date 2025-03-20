﻿#include "framework.h"
#include "KakaoTalkAdGuard.h"

// Global variables
HINSTANCE       hInst;
LPWSTR          szCmdLine = 0;
WCHAR           szTitle[MAX_LOADSTRING];
WCHAR           szWindowClass[MAX_LOADSTRING];
UINT            updateRate = 100;
BOOL            autoStartup = false;
BOOL            hideTrayIcon = false;
NOTIFYICONDATA  nid = {sizeof(nid)};
BOOL            bPortable = true;

// Forward-declaration
ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
BOOL             CheckMultipleExecution(HINSTANCE hInst, HWND hWnd, WCHAR szWindowClass[MAX_LOADSTRING]);
BOOL             HideTrayIcon(HINSTANCE hInst, HWND hWnd, NOTIFYICONDATA nid);
BOOL             ToggleStartup(HWND hWnd);
BOOL             CheckStartup(HINSTANCE hInst, HWND hWnd);
BOOL             CreateTrayIcon(HWND hWnd, NOTIFYICONDATA* nid);
BOOL             DeleteTrayIcon(NOTIFYICONDATA nid);
VOID             ShowContextMenu(HWND hwnd, POINT pt);
BOOL             ShowNewUpdateBalloon();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK    TimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTimer);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	// Parse command-line
	if (lpCmdLine != L"") {
		szCmdLine = lpCmdLine;
	}
	
	// Load resources
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	if (lstrcmpW(szCmdLine, L"--restore_tray") == 0) {
		LoadStringW(hInstance, IDC_KAKAOTALKADGUARD_RESTORETRAY, szWindowClass, MAX_LOADSTRING);
	} else {
		LoadStringW(hInstance, IDC_KAKAOTALKADGUARD, szWindowClass, MAX_LOADSTRING);
	}

	// Initialize Window
	MyRegisterClass(hInstance);
	if (!InitInstance(hInstance, nCmdShow)) { return FALSE; }

	// Message loop
	MSG msg; while (GetMessage(&msg, nullptr, 0, 0)) { // Wait for new message
		TranslateMessage(&msg); // Translate WM_KEYDOWN to WM_CHAR
		DispatchMessage(&msg); // Dispatch to WndProc
	} return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOGO));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_LOGO));
	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance;
	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) { return FALSE; }
	/*ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);*/
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { // Called by kernel
	static HANDLE hTimer;
	static WCHAR appName[64];
	BOOL bClose = FALSE;
	BOOL bRestoretray = FALSE;

	switch (message) {
	case WM_CREATE:
		// Preprocess arguments
		if (lstrcmpW(szCmdLine, L"--startup") == 0) {
			bPortable = false;
		} else if (lstrcmpW(szCmdLine, L"--restore_tray") == 0) {
			bRestoretray = TRUE;
			HKEY key; DWORD dwDisp;
			if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &dwDisp) == ERROR_SUCCESS) {
				DWORD value = 0;
				RegSetValueExW(key, L"HideTrayIcon", 0, REG_DWORD, (const BYTE*) &value, sizeof(value));
			}
			RegCloseKey(key);
			HWND hKakaoTalkAdGuardMain = FindWindow(L"KakaoTalkAdGuard", NULL);
			if (hKakaoTalkAdGuardMain) {
				SendMessage(hKakaoTalkAdGuardMain, WM_RECHECK, NULL, NULL);
			}
			PostQuitMessage(0);
		}

		bClose = CheckMultipleExecution(hInst, hWnd, szWindowClass);
		hTimer = (HANDLE) SetTimer(hWnd, 1, updateRate, (TIMERPROC) TimerProc);
		CheckStartup(hInst, hWnd);
		if (!hideTrayIcon && !bClose && !bRestoretray) {
			CreateTrayIcon(hWnd, &nid);
		}
		break;
	case WM_NOTIFYCALLBACK:
		switch (LOWORD(lParam)) {
		case NIN_SELECT:
		case WM_CONTEXTMENU:
		{
			POINT const pt = {LOWORD(wParam), HIWORD(wParam)};
			ShowContextMenu(hWnd, pt);
		}
		break;
		}
		break;
	case WM_RECHECK:
		CheckStartup(hInst, hWnd);
		if (!hideTrayIcon) {
			CreateTrayIcon(hWnd, &nid);
		}
		break;
	case WM_INITMENU:
		if (autoStartup)
			CheckMenuItem((HMENU) wParam, IDM_STARTONSYSTEMSTARTUP, MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem((HMENU) wParam, IDM_STARTONSYSTEMSTARTUP, MF_BYCOMMAND | MF_UNCHECKED);

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDM_HIDETRAYICON:
			HideTrayIcon(hInst, hWnd, nid);
			break;
		case IDM_STARTONSYSTEMSTARTUP:
			ToggleStartup(hWnd);
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
		KillTimer(hWnd, 1);
		DeleteTrayIcon(nid);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

BOOL CheckStartup(HINSTANCE hInst, HWND hWnd) {
	// Check autoStartup
	HKEY key; DWORD dwDisp;
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_RUN, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);

	if (RegQueryValueExW(key, L"KakaoTalkAdGuard", 0, NULL, 0, NULL) == NO_ERROR) {
		autoStartup = true;
	} else {
		autoStartup = false;
	}
	RegCloseKey(key);
	SendMessage(hWnd, WM_INITMENU, 0, 0);

	// Check HideTrayIcon
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &key, &dwDisp);
	DWORD dwType = REG_DWORD;
	DWORD dwValue = 0;
	DWORD dwDataSize = sizeof(DWORD);
	DWORD ret = RegQueryValueExW(key, L"HideTrayIcon", 0, &dwType, (LPBYTE) &dwValue, &dwDataSize);
	if (dwValue) {
		hideTrayIcon = TRUE;
	} else {
		hideTrayIcon = FALSE;
	}
	RegCloseKey(key);
	return 0;
}

BOOL ToggleStartup(HWND hWnd) {
	HKEY key; DWORD dwDisp;
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_RUN, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);

	if (RegQueryValueExW(key, L"KakaoTalkAdGuard", 0, NULL, 0, NULL) == NO_ERROR) {
		RegDeleteValueW(key, L"KakaoTalkAdGuard");
		autoStartup = false;
	} else {
		WCHAR szFileName[MAX_PATH];
		WCHAR szFileNameFinal[MAX_PATH] = L"\"";
		GetModuleFileName(NULL, szFileName, MAX_PATH);
		lstrcatW(szFileNameFinal, szFileName);
		lstrcatW(szFileNameFinal, L"\"");
		lstrcatW(szFileNameFinal, L" --startup");
		RegSetValueExW(key, L"KakaoTalkAdGuard", 0, REG_SZ, (LPBYTE) szFileNameFinal, (lstrlenW(szFileNameFinal) + 1) * sizeof(WCHAR));
		autoStartup = true;
	}
	RegCloseKey(key);
	return 0;
}

BOOL HideTrayIcon(HINSTANCE hInst, HWND hWnd, NOTIFYICONDATA nid) {
	WCHAR msgboxHideTray[MAX_LOADSTRING];
	LoadStringW(hInst, IDS_MSGBOX_HIDETRAY, msgboxHideTray, MAX_LOADSTRING);
	MessageBox(hWnd, msgboxHideTray, NULL, MB_ICONWARNING);
	HKEY key; DWORD dwDisp;
	LSTATUS ret;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &dwDisp) == ERROR_SUCCESS) {
		DWORD value = 1;
		ret = RegSetValueExW(key, L"HideTrayIcon", 0, REG_DWORD, (const BYTE*) &value, sizeof(value));
	}
	RegCloseKey(key);
	DeleteTrayIcon(nid);
	return 0;
}

BOOL CheckMultipleExecution(HINSTANCE hInst, HWND hWnd, WCHAR szWindowClass[MAX_LOADSTRING]) {
	CreateMutex(NULL, TRUE, szWindowClass);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		WCHAR msgboxIsRunning[MAX_LOADSTRING];
		LoadStringW(hInst, IDS_MSGBOX_ISRUNNING, msgboxIsRunning, MAX_LOADSTRING);
		MessageBox(hWnd, msgboxIsRunning, NULL, MB_ICONWARNING);
		PostQuitMessage(0);
		return 1;
	}
	return 0;
}

BOOL ShowNewUpdateBalloon() {
	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_INFO;
	LoadStringW(hInst, IDS_NEWUPDATE_TITLE, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
	LoadStringW(hInst, IDS_NEWUPDATE_CONTENT, nid.szInfo, ARRAYSIZE(nid.szInfo));
	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

BOOL CreateTrayIcon(HWND hWnd, NOTIFYICONDATA* nid) {
	nid->hWnd = hWnd;
	nid->uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
	LoadString(hInst, IDS_APP_NAME, nid->szTip, ARRAYSIZE(nid->szTip));
	nid->hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGO));
	nid->uCallbackMessage = WM_NOTIFYCALLBACK;
	Shell_NotifyIcon(NIM_ADD, nid);
	nid->uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, nid);
}

BOOL DeleteTrayIcon(NOTIFYICONDATA nid) {
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd, POINT pt) {
	HMENU hMenu;
	WCHAR szAppName[MAX_LOADSTRING];
	WCHAR szVersion[MAX_LOADSTRING];
	WCHAR szFullAppName[MAX_LOADSTRING];
	szFullAppName[0] = L'\0';
	LoadStringW(hInst, IDS_APP_NAME, szAppName, MAX_LOADSTRING);
	LoadStringW(hInst, IDS_APP_VERSION, szVersion, MAX_LOADSTRING);
	wcscpy_s(szFullAppName, szAppName);
	wcscat_s(szFullAppName, L" ");
	wcscat_s(szFullAppName, szVersion);

	if (bPortable) {
		hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_TRAY_CONTEXTMENU_PORTABLE));
		ModifyMenuW(hMenu, ID__APP_TITLE, MF_BYCOMMAND | MF_STRING | MF_DISABLED, ID__APP_TITLE, (LPCWSTR) szFullAppName);
	} else {
		hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_TRAY_CONTEXTMENU));
		ModifyMenuW(hMenu, ID__APP_TITLE, MF_BYCOMMAND | MF_STRING | MF_DISABLED, ID__APP_TITLE, (LPCWSTR)szFullAppName);
	}
	HMENU hSubMenu = GetSubMenu(hMenu, 0);
	SetForegroundWindow(hwnd); // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
	// respect menu drop alignment
	UINT uFlags = TPM_RIGHTBUTTON;
	if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
		uFlags |= TPM_RIGHTALIGN;
	} else {
		uFlags |= TPM_LEFTALIGN;
	}
	TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
	DestroyMenu(hMenu);
}

/*
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lparam) {
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if ((DWORD) lparam == pid) {
		return FALSE; // found
	} else {
		return TRUE; // continue
	}
}
*/

HWND hKakaoTalkMain;
HWND hAdFit;
RECT RectKakaoTalkMain;

/*
BOOL CALLBACK EnumWindowProc(HWND hwnd, LPARAM lParam) {
	WCHAR className[256];
	GetClassName(hwnd, className, 256);

	WCHAR windowText[256] = L"";
	GetWindowText(hwnd, windowText, 256);

	if (wcscmp(className, L"EVA_Window_Dblclk") == 0) {
		HWND hBannerAdChild = FindWindowEx(hwnd, NULL, L"BannerAdContainer", L"");
		if (hBannerAdChild != NULL) {
			ShowWindow(hwnd, SW_HIDE);
			return TRUE;
		}
	}	
	return TRUE;
}
*/

BOOL CALLBACK EnumChildProcFromMainWin(HWND hwnd, LPARAM lParam) {
	WCHAR className[256] = L"";
	WCHAR windowText[256] = L"";
	GetClassName(hwnd, className, 256);
	GetWindowText(hwnd, windowText, 256);

	if (wcscmp(className, L"EVA_ChildWindow") == 0) {
		HWND parentHandle = GetParent(hwnd);
		if (wcsncmp(windowText, L"OnlineMainView_", 15) == 0) { // Expand chat widget to empty space
			SetWindowPos(hwnd, HWND_TOP, 0, 0, (RectKakaoTalkMain.right - RectKakaoTalkMain.left), (RectKakaoTalkMain.bottom - RectKakaoTalkMain.top - 32), SWP_NOMOVE);
		}
		else if (parentHandle == hKakaoTalkMain) { // 메인 창 광고 컨테이너 숨기기
			ShowWindow(hwnd, SW_HIDE);
			SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE);
		}
		return TRUE;
	}
	if (wcsncmp(windowText, L"LockModeView_", 13) == 0) { // Expand numpad in Lockdown mode
		HWND hLockdownNumpad = FindWindowEx(hwnd, NULL, L"EVA_ChildWindow", L"");
		if (hLockdownNumpad != NULL)
			SetWindowPos(hwnd, HWND_TOP, 0, 0, (RectKakaoTalkMain.right - RectKakaoTalkMain.left), (RectKakaoTalkMain.bottom - RectKakaoTalkMain.top), SWP_NOMOVE);
	}
/*
	if (wcscmp(className, L"BannerAdWnd") == 0) {
		ShowWindow(hwnd, SW_HIDE);
		return TRUE;
	}
	if (wcscmp(className, L"RichPopWnd") == 0) {
		ShowWindow(hwnd, SW_HIDE);
		return TRUE;
	}
*/
	if (wcscmp(className, L"EVA_VH_ListControl_Dblclk") == 0) { // 광고 제거 후 채팅 목록 다시 그리기
		InvalidateRect(hwnd, NULL, TRUE);
		return TRUE;
	}

	return TRUE;
}

BOOL CALLBACK HideAdWindows(HWND hwnd, LPARAM lParam) {
	ShowWindow(hwnd, SW_HIDE);
	SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE);
	return TRUE;
}

BOOL CALLBACK EnumChildProcFromSubWin(HWND hwnd, LPARAM lParam) {
	WCHAR className[256] = L"";
	WCHAR windowText[256] = L"";
	GetClassName(hwnd, className, 256);
	GetWindowText(hwnd, windowText, 256);

	if (wcscmp(className, L"BannerAdWnd") == 0 || wcscmp(className, L"BannerAdContainer") == 0 || wcscmp(className, L"RichPopWnd") == 0) {
		ShowWindow(hwnd, SW_HIDE);
		return TRUE;
	}
	if (wcscmp(className, L"Chrome_WidgetWin_1") == 0) {
		if (wcscmp(windowText, L"MOMENT 광고") == 0) { // 메인 창 광고 숨기기
			HWND parentHandle = GetParent(hwnd);
			EnumChildWindows(hwnd, HideAdWindows, NULL);
			ShowWindow(hwnd, SW_HIDE);
			ShowWindow(parentHandle, SW_HIDE);
			parentHandle = GetParent(parentHandle);
			ShowWindow(parentHandle, SW_HIDE);
			parentHandle = GetParent(parentHandle);
			ShowWindow(parentHandle, SW_HIDE);
			parentHandle = GetParent(parentHandle);
			ShowWindow(parentHandle, SW_HIDE);
			parentHandle = GetParent(parentHandle);
			ShowWindow(parentHandle, SW_HIDE);
			SetWindowPos(parentHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE);
//			return FALSE;
		}
//		return TRUE;
	}
	return TRUE;
}

VOID CALLBACK TimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTimer) {
	switch (idEvent) {
	case 1: // Remove KakaoTalk ADs
		// Find main handle
		const WCHAR* kakaoTalkNames[] = {L"카카오톡", L"カカオトーク", L"KakaoTalk"};
		int numNames = sizeof(kakaoTalkNames) / sizeof(kakaoTalkNames[0]);
		for (int i = 0; i < numNames; ++i) {
			hKakaoTalkMain = FindWindow(L"EVA_Window_Dblclk", kakaoTalkNames[i]);
			if (hKakaoTalkMain != NULL)
				break;
		}
		if (hKakaoTalkMain != NULL) // 카카오톡이 실행되어 있지 않으면 진행 안 함
		{
			// Scan ADs recursive
			hAdFit = FindWindow(L"EVA_Window_Dblclk", L"");
			if (hAdFit != NULL && IsWindowVisible(hAdFit)) { // 광고 창이 보일 때만 진행
				EnumChildWindows(hAdFit, EnumChildProcFromSubWin, NULL);
			}
			if (IsWindowVisible(hKakaoTalkMain)) { // 카카오톡 메인 창이 보일 때만 진행
				GetWindowRect(hKakaoTalkMain, &RectKakaoTalkMain);
				EnumChildWindows(hKakaoTalkMain, EnumChildProcFromMainWin, NULL);
			}
			//EnumWindows(EnumWindowProc, NULL);
/*
			// Block popup AD
			DWORD pid_main = 0;
			DWORD pid_popup = 0;
			HWND hPopupWnd = FindWindow(L"RichPopWnd", L"");
			GetWindowThreadProcessId(hKakaoTalkMain, &pid_main);
			GetWindowThreadProcessId(hPopupWnd, &pid_popup);
			if (pid_main == pid_popup)
				ShowWindow(hPopupWnd, SW_HIDE);
*/
		}
		break;
	}
}