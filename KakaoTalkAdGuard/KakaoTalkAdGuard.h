#pragma once

#include "resource.h"

#define MAX_LOADSTRING 100
UINT const      WM_NOTIFYCALLBACK = WM_APP + 1;
UINT const      WM_RECHECK = WM_APP + 2;
WCHAR const     REG_RUN[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
WCHAR const     REG_CFG[] = L"SOFTWARE\\KakaoTalkAdGuard";
WCHAR const		REG_HIDETRAYICON[] = L"HideTrayIcon";
WCHAR const		REG_SHOWUPDATEBALLOON[] = L"ShowUpdateBalloon";
WCHAR const     REG_OPENUPDATEPAGE[] = L"OpenUpdatePage";
