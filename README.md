# ![](Docs/KakaoTalkAdGuard.png) KakaoTalk AdGuard

Ad removal tool for Windows KakaoTalk

## Features

- No framework libraries required
- Support Windows and Windows Server including all language versions
- Support Autorun on system startup (installation version only)
- Support Installer and Uninstaller

## Command Line Arguments

- --startup: Use the tray menu for the installation version
- --restore_tray: Show the hidden tray icon

## Download

- Before or after installation, Specify the installation directory path (default: `%appdata%\KakaoTalkAdGuard`) as an exception path in your antivirus software settings. Because this binary file is not well known, antivirus software may detect it as malware.
- [Go to Release page to download](https://github.com/kndol/KakaoTalkAdGuard/releases)

## Known issues

- On Windows with HiDPI, the trayicon menu is displayed in an abnormal position.

## Preview

![Highlights](kakaotalk-adguard.loopback.kr/Highlights.png)

## Release notes

### 1.0.0.15

<sup>2025년 9월 27일</sup>

- 설치 버전의 경우 업데이트 알림 사용 여부를 트레이 메뉴에서 선택할 수 있게 함
- 업데이트 알림을 풍선 알림으로 표시하기 추가

![image](https://private-user-images.githubusercontent.com/4049720/494483760-b2bfef56-e649-43d9-a8df-a55554151af6.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NTg5MDE4OTMsIm5iZiI6MTc1ODkwMTU5MywicGF0aCI6Ii80MDQ5NzIwLzQ5NDQ4Mzc2MC1iMmJmZWY1Ni1lNjQ5LTQzZDktYThkZi1hNTU1NTQxNTFhZjYucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI1MDkyNiUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNTA5MjZUMTU0NjMzWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9ZDY5MjY5YWE2ZjU0ZTU3NTFhOTcwOGE0YjFiNGVmZTM4NmNmOWM0NGI2YjY2MjNlYmJkYjA3MmNiMzc2NTk3ZCZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QifQ.W9oSw_zMb3dfSnWDXPWzPCs6MSW6maHawfPEjp7RpnA)
![image](https://private-user-images.githubusercontent.com/4049720/494483750-afef21a4-3ec8-4b42-a5f4-fe76a763b01d.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NTg5MDE4OTMsIm5iZiI6MTc1ODkwMTU5MywicGF0aCI6Ii80MDQ5NzIwLzQ5NDQ4Mzc1MC1hZmVmMjFhNC0zZWM4LTRiNDItYTVmNC1mZTc2YTc2M2IwMWQucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI1MDkyNiUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNTA5MjZUMTU0NjMzWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9NTUyMTM2YTNlZTU0MGFkZTRhNzIwMzBiY2VkYzczNTI0NGE2MWMwMzhiMTYwMjdkYzU3ZDRjOTc1MTMzOTIwNiZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QifQ.IYd87LXQYKsptnT5bzYK0t5dnvuXLr5111Xjk1tlbA8)
![image](https://private-user-images.githubusercontent.com/4049720/494483769-74180e9b-4088-4909-aedc-da97b6515313.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NTg5MDE4OTMsIm5iZiI6MTc1ODkwMTU5MywicGF0aCI6Ii80MDQ5NzIwLzQ5NDQ4Mzc2OS03NDE4MGU5Yi00MDg4LTQ5MDktYWVkYy1kYTk3YjY1MTUzMTMucG5nP1gtQW16LUFsZ29yaXRobT1BV1M0LUhNQUMtU0hBMjU2JlgtQW16LUNyZWRlbnRpYWw9QUtJQVZDT0RZTFNBNTNQUUs0WkElMkYyMDI1MDkyNiUyRnVzLWVhc3QtMSUyRnMzJTJGYXdzNF9yZXF1ZXN0JlgtQW16LURhdGU9MjAyNTA5MjZUMTU0NjMzWiZYLUFtei1FeHBpcmVzPTMwMCZYLUFtei1TaWduYXR1cmU9YTg1YTZjZGRlMjU2MTVjMzkxYWI2ZDFkZTA3ZWY1OGRlNzk2ZWZjZjNjYTUzYmE3MzViNmJhMDU2ODYwNzE1OSZYLUFtei1TaWduZWRIZWFkZXJzPWhvc3QifQ.iUoTE4RgU0bOtgEDSOjfuAH4xFvBsDe4pt1oQFBJbKU)

### 1.0.0.14

<sup>2025년 9월 25일</sup>

- 메뉴 및 메시지 한글화
- 새 버전 검사 기능 추가

### 1.0.0.13

<sup>Mar. 20, 2025</sup>

- Fixed an issue where traces of the ad area could be visible at the bottom of the main window after removing an ad
- Prevents the ad area in the main window from appearing even after exiting KakaoTalk AdGuard
- Optimize internal logic

### 1.0.0.12

<sup>Mar. 09, 2025</sup>

- Fixed an issue where the chromium browser window disappears when KakaoTalk is not running but KakaoTalkAdGuard is running
- Fixed an issue where KakaoTalk disappeared when opening the notification window of KakaoTalk.
- Fixed by KnDol

### 1.0.0.11

<sup>Dec. 19, 2024</sup>

- Fixed an issue with ADs appearing in version 4.3.0.4263

### 1.0.0.10

<sup>May 9, 2024</sup>

- Fixed an issue where ads were not blocked when opening a KakaoTalk window and message notification windows were open at the same time

### 1.0.0.9

<sup>May 4, 2024</sup>

- Fixed an issue that updated profiles disappeared

### 1.0.0.8

<sup>Apr. 27, 2024</sup>

- Hotfix for the lockdown setting window not appearing issue

### 1.0.0.7

<sup>Apr. 27, 2024</sup>

- Support for multilingual versions of KakaoTalk
- Changed banner ad blocking criteria
- Removed white box at lockdown mode
- Code refactoring

### 1.0.0.6

<sup>Apr. 22, 2024</sup>

- Hotfix for the profile window not appearing issue

### 1.0.0.5

<sup>Apr. 22, 2024</sup>

- Support KakaoTalk version 4

### 1.0.0.4

<sup>Apr. 20, 2024</sup>

- Hotfix for KakaoTalk version 4 (experimental)

### 1.0.0.3

<sup>Jun. 23, 2023</sup>

- Support 32-bit architecture
- Support portable version trayicon context menu
- Changed restoring trayicon mechanism

### 1.0.0.2

<sup>Jun. 1, 2023</sup>

- Improved stability of trayicon
- Improved stability for popup ad removal mechanism
- Changed context menu items for trayicon
- Fixed an issue that a registry key was not deleted when uninstalling
- Update icon for uninstallation
- Minimized Windows UAC permissions
- Changed installation mechanism to close running processes
- Added version attributes and uninstall information

### 1.0.0.1

<sup>May 30, 2023</sup>

- Fixed issue where the send button was removed when sending files in chat room.

### 1.0.0.0

<sup>May 29, 2023</sup>

- Initial release
