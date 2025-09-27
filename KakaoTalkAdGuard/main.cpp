#include "framework.h"
#include "KakaoTalkAdGuard.h"
#include <winhttp.h>
#include <string>
#include <vector>
#pragma comment(lib, "winhttp.lib")

// Global variables
HINSTANCE       hInst;
HWND            g_hWnd = NULL;
LPWSTR          szCmdLine = 0;
WCHAR           szTitle[MAX_LOADSTRING];
WCHAR           szWindowClass[MAX_LOADSTRING];
UINT            updateRate = 100;
BOOL            autoStartup = false;
BOOL            hideTrayIcon = false;
BOOL            showUpdateBalloon = true;
BOOL            openUpdatePage = true;
NOTIFYICONDATA  nid = {sizeof(nid)};
BOOL            bPortable = true;

// Forward-declaration
ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
BOOL             CheckMultipleExecution(HINSTANCE hInst, HWND hWnd, WCHAR szWindowClass[MAX_LOADSTRING]);
BOOL             HideTrayIcon(HINSTANCE hInst, HWND hWnd, NOTIFYICONDATA nid);
BOOL             ToggleStartup(HWND hWnd);
BOOL             ToggleShowBalloon(HWND hWnd);
BOOL             ToggleOpenPage(HWND hWnd);
BOOL             CheckStartup(HINSTANCE hInst, HWND hWnd);
BOOL             CreateTrayIcon(HWND hWnd, NOTIFYICONDATA* nid);
BOOL             DeleteTrayIcon(NOTIFYICONDATA nid);
VOID             ShowContextMenu(HWND hwnd, POINT pt);
BOOL             ShowNewUpdateBalloon(LPCWSTR title, LPCWSTR msg);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK    TimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTimer);
// 스레드에서 실행될 함수
DWORD WINAPI GitHubCheckThread(LPVOID lpParam);

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
	g_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	if (!g_hWnd) { return FALSE; }
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
				RegSetValueExW(key, REG_HIDETRAYICON, 0, REG_DWORD, (const BYTE*) &value, sizeof(value));
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
		if (showUpdateBalloon || openUpdatePage)
			CreateThread(NULL, 0, GitHubCheckThread, (LPVOID)hWnd, 0, NULL);
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
		CheckMenuItem((HMENU) wParam, IDM_STARTONSYSTEMSTARTUP, MF_BYCOMMAND | autoStartup ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, IDM_SHOWUPDATEBALLOON, MF_BYCOMMAND | showUpdateBalloon ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, IDM_OPENNEWVERSIONPAGE, MF_BYCOMMAND | openUpdatePage ? MF_CHECKED : MF_UNCHECKED);

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDM_HIDETRAYICON:
			HideTrayIcon(hInst, hWnd, nid);
			break;
		case IDM_STARTONSYSTEMSTARTUP:
			ToggleStartup(hWnd);
			break;
		case IDM_SHOWUPDATEBALLOON:
			ToggleShowBalloon(hWnd);
			break;
		case IDM_OPENNEWVERSIONPAGE:
			ToggleOpenPage(hWnd);
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
	RegQueryValueExW(key, REG_HIDETRAYICON, 0, &dwType, (LPBYTE) &dwValue, &dwDataSize);
	hideTrayIcon = dwValue ? TRUE  : FALSE;

	dwValue = 1;
	RegQueryValueExW(key, REG_SHOWUPDATEBALLOON, 0, &dwType, (LPBYTE)&dwValue, &dwDataSize);
	showUpdateBalloon = dwValue ? TRUE : FALSE;

	dwValue = 1;
	RegQueryValueExW(key, REG_OPENUPDATEPAGE, 0, &dwType, (LPBYTE)&dwValue, &dwDataSize);
	openUpdatePage = dwValue ? TRUE : FALSE;

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

BOOL ToggleShowBalloon(HWND hWnd) {
	HKEY key; DWORD dwDisp;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &dwDisp) == ERROR_SUCCESS) {
		showUpdateBalloon = !showUpdateBalloon;
		DWORD value = showUpdateBalloon;// ? 1 : 0;
		RegSetValueExW(key, REG_SHOWUPDATEBALLOON, 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
	}
	RegCloseKey(key);
	return 0;
}

BOOL ToggleOpenPage(HWND hWnd) {
	HKEY key; DWORD dwDisp;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &dwDisp) == ERROR_SUCCESS) {
		openUpdatePage = !openUpdatePage;
		DWORD value = openUpdatePage;
		RegSetValueExW(key, REG_OPENUPDATEPAGE, 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
	}
	RegCloseKey(key);
	return 0;
}

BOOL HideTrayIcon(HINSTANCE hInst, HWND hWnd, NOTIFYICONDATA nid) {
	WCHAR msgboxHideTray[MAX_LOADSTRING];
	LoadStringW(hInst, IDS_MSGBOX_HIDETRAY, msgboxHideTray, MAX_LOADSTRING);
	MessageBox(hWnd, msgboxHideTray, NULL, MB_ICONWARNING);
	HKEY key; DWORD dwDisp;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &dwDisp) == ERROR_SUCCESS) {
		DWORD value = 1;
		RegSetValueExW(key, REG_HIDETRAYICON, 0, REG_DWORD, (const BYTE*) &value, sizeof(value));
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

BOOL ShowNewUpdateBalloon(LPCWSTR title, LPCWSTR msg) {
	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.cbSize = sizeof(nid);
	nid.hWnd = g_hWnd;
	nid.uFlags = NIF_MESSAGE | NIF_INFO | NIF_ICON;
	nid.dwInfoFlags = NIIF_INFO;        // 풍선 알림아이콘 종류
	nid.hIcon = LoadIconW(hInst, MAKEINTRESOURCE(IDI_LOGO));
	nid.uCallbackMessage = WM_NOTIFYCALLBACK;
	nid.uTimeout = 5000;
	lstrcpyW(nid.szInfo, msg);
	lstrcpyW(nid.szInfoTitle, title);
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

// GitHub Releases API에서 latest 정보를 가져오는 간단한 함수
static bool HttpGetUtf8(const wchar_t* host, const wchar_t* path, std::string& outBody) {
	HINTERNET hSession = WinHttpOpen(L"KakaoTalkAdGuard-Agent/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return false;
	HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

	// GitHub API 요구 헤더
	WinHttpAddRequestHeaders(hRequest, L"User-Agent: KakaoTalkAdGuard\r\n", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
	WinHttpAddRequestHeaders(hRequest, L"Accept: application/vnd.github.v3+json\r\n", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);

	BOOL bSend = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	if (!bSend) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
	if (!WinHttpReceiveResponse(hRequest, NULL)) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

	std::vector<char> buffer;
	DWORD dwRead = 0;
	do {
		DWORD dwAvailable = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &dwAvailable)) break;
		if (dwAvailable == 0) break;
		std::vector<char> chunk(dwAvailable);
		if (!WinHttpReadData(hRequest, chunk.data(), dwAvailable, &dwRead) || dwRead == 0) break;
		buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + dwRead);
	} while (dwRead != 0);

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	// GitHub 응답은 UTF-8이므로 그대로 std::string에 저장
	outBody.assign(buffer.begin(), buffer.end());
	return true;
}

// GitHub에서 latest tag_name을 파싱하여 현재 버전과 비교하고 알림을 띄움
BOOL CheckGitHubLatestUpdate(HWND hWnd) {
	const wchar_t* host = L"api.github.com";
	// 저장소: kndol/KakaoTalkAdGuard — 필요하면 변경
	const wchar_t* path = L"/repos/kndol/KakaoTalkAdGuard/releases/latest";
	const wchar_t* url = L"https://github.com/kndol/KakaoTalkAdGuard/releases/latest";
	
	std::string body;
	if (!HttpGetUtf8(host, path, body)) return FALSE;

	// 간단한 문자열 파싱: "tag_name":"v1.2.3"
	const std::string key = "\"tag_name\":";
	size_t pos = body.find(key);
	if (pos == std::string::npos) return FALSE;
	pos = body.find('"', pos + key.length());
	if (pos == std::string::npos) return FALSE;
	size_t start = pos + 1;
	size_t end = body.find('"', start);
	if (end == std::string::npos) return FALSE;
	std::string latestTag = body.substr(start, end - start);

	// 현재 애플리케이션 버전을 리소스에서 읽어서 비교 (wide -> utf8)
	WCHAR currentVersionW[64] = L"";
	LoadStringW(hInst, IDS_APP_VERSION, currentVersionW, ARRAYSIZE(currentVersionW));
	// 변환: WCHAR(UTF-16) -> UTF-8
	int utf8len = WideCharToMultiByte(CP_UTF8, 0, currentVersionW, -1, NULL, 0, NULL, NULL);
	std::string currentVersion;
	if (utf8len > 0) {
		currentVersion.resize(utf8len - 1);
		WideCharToMultiByte(CP_UTF8, 0, currentVersionW, -1, &currentVersion[0], utf8len, NULL, NULL);
	}

#ifdef _DEBUG
	currentVersion = "1.0.0.15"; // 테스트
	latestTag = "1.0.0.16";
#endif // _DEBUG

	if (!currentVersion.empty() && latestTag > currentVersion) {
		// 추가 상세 알림: 메시지 박스로 새 버전과 다운로드 페이지 링크 제공
		std::wstring msg, balloonMsg;
		std::wstring latestTagW;
		{
			// UTF-8 latestTag -> UTF-16
			int wlen = MultiByteToWideChar(CP_UTF8, 0, latestTag.c_str(), -1, NULL, 0);
			latestTagW.resize((wlen > 0) ? (wlen - 1) : 0);
			if (wlen > 0) MultiByteToWideChar(CP_UTF8, 0, latestTag.c_str(), -1, &latestTagW[0], wlen);
		}
		WCHAR msgNewTitle[MAX_LOADSTRING];
		WCHAR msgNew1[MAX_LOADSTRING];
		WCHAR msgNew2[MAX_LOADSTRING];
		WCHAR msgCurVer[MAX_LOADSTRING];
		WCHAR msgNewVer[MAX_LOADSTRING];
		LoadStringW(hInst, IDS_NEWUPDATE_TITLE, msgNewTitle , MAX_LOADSTRING);
		LoadStringW(hInst, IDS_NEWUPDATE_CONTENT, msgNew1, MAX_LOADSTRING);
		LoadStringW(hInst, IDS_NEWUPDATE_OPENPAGE, msgNew2, MAX_LOADSTRING);
		LoadStringW(hInst, IDS_CUR_VERSION, msgCurVer, MAX_LOADSTRING);
		LoadStringW(hInst, IDS_NEW_VERSION, msgNewVer, MAX_LOADSTRING);

		balloonMsg = msgNew1;
		balloonMsg += L"\r\n\r\n";
		balloonMsg += msgCurVer + std::wstring(currentVersion.begin(), currentVersion.end()) + L"\r\n";
		balloonMsg += msgNewVer + latestTagW;
		msg = balloonMsg + L"\r\n\r\n";
		msg += msgNew2;

		// release 페이지 열기
		// 새 버전 존재: 트레이 풍선(정의된 함수 사용) 또는 메시지 박스
		if (showUpdateBalloon)
			ShowNewUpdateBalloon(L"", balloonMsg.c_str());
		if (openUpdatePage && MessageBoxW(hWnd, msg.c_str(), msgNewTitle, MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST) == IDYES)
			ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
		return TRUE;
	}

	return FALSE;
}

// 스레드 엔트리 포인트
DWORD WINAPI GitHubCheckThread(LPVOID lpParam) {
	HWND hWnd = (HWND)lpParam;
	CheckGitHubLatestUpdate(hWnd);
	return 0;
}