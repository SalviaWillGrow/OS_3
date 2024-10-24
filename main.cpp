#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include "Commctrl.h"

BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

bool Terminate = false;
HWND hMainWnd = 0;

HANDLE hClockThread = 0;
bool ClockPaused = false;
HWND hClockText = 0;

HFONT fontHandle;
HBRUSH hbrBkgnd = NULL;

HANDLE pausePic;
HANDLE playPic;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

	pausePic = LoadImageA(
		NULL,
		"pause.bmp",
		IMAGE_BITMAP,
		38,
		38,
		LR_LOADFROMFILE
	);

	playPic = LoadImageA(
		NULL,
		"play.bmp",
		IMAGE_BITMAP,
		38,
		38,
		LR_LOADFROMFILE
	);

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, DLGPROC(MainWndProc));

	return 0;
}

DWORD WINAPI ClockThread(LPVOID lpParameter) {
	while (!Terminate) {
		char timestr[9];
		SYSTEMTIME currentTime;
		GetLocalTime(&currentTime);
		sprintf_s(timestr, "%.2d:%.2d:%.2d",
			currentTime.wHour, currentTime.wMinute, currentTime.wSecond);
		SetDlgItemText(hMainWnd, IDC_CLOCK, timestr);
		Sleep(250);
	}
	return 0;
}

unsigned __int64 SystemTimeToInt(SYSTEMTIME Time) {
	FILETIME ft;
	SystemTimeToFileTime(&Time, &ft);
	ULARGE_INTEGER fti;
	fti.HighPart = ft.dwHighDateTime;
	fti.LowPart = ft.dwLowDateTime;
	return fti.QuadPart;
}

bool RunProcess() {

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;

	CHAR command[99];
	GetDlgItemTextA(
		hMainWnd,
		IDC_COMMANDLINE,
		command,
		99);

	if (CreateProcessA(
		NULL,
		command,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi)) {

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		
		return TRUE;
	}
	return FALSE;
}

DWORD WINAPI ScheduleThread(LPVOID lpParameter) {

	SYSTEMTIME plannedTime;
	SendDlgItemMessageA(
		hMainWnd, IDC_TIME, DTM_GETSYSTEMTIME, 0,
		(LPARAM)&plannedTime
	);

	char timestr[9];
	sprintf_s(timestr, "%.2d:%.2d:%.2d",
		plannedTime.wHour, plannedTime.wMinute, plannedTime.wSecond
	);
	SendDlgItemMessageA(
		hMainWnd, IDC_TIMELIST, LB_ADDSTRING,
		0, (LPARAM)&timestr
	);

	SYSTEMTIME currentTime;
	int plannedTimeI = SystemTimeToInt(plannedTime);

	while (!Terminate) {

		GetLocalTime(&currentTime);
		int currentTimeI = SystemTimeToInt(currentTime);

		if (currentTimeI > plannedTimeI) {
			char timestr[9];
			sprintf_s(timestr, "%.2d:%.2d:%.2d",
				plannedTime.wHour, plannedTime.wMinute, plannedTime.wSecond
			);
			LRESULT lineIndex = SendDlgItemMessageA(
				hMainWnd, IDC_TIMELIST, LB_FINDSTRINGEXACT,
				-1, (LPARAM)&timestr
			);
			if (lineIndex != LB_ERR) {
				RunProcess();
				SendDlgItemMessageA(
					hMainWnd, IDC_TIMELIST, LB_DELETESTRING,
					lineIndex, 0
				);
			}
			return 0;
		}

		Sleep(10);
	}

	return 0;
}

BOOL BrowseFileName(HWND hWindow, char* ptrFileName) {

	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWindow;
	ofn.lpstrFilter = "All Files\0*.*\0\0";
	ofn.lpstrFile = ptrFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = "exe";
	ofn.Flags = OFN_FILEMUSTEXIST;

	return GetOpenFileName(&ofn);
}

BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_INITDIALOG:
	{
		hMainWnd = hWnd;
		hClockThread = CreateThread(NULL, 0, ClockThread, NULL, 0, NULL);
		SetDlgItemText(hMainWnd, IDC_COMMANDLINE, "notepad.exe");

		fontHandle = CreateFontA(
			48, 0, 0, 0,
			700, false, false, false,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH, TEXT("Impact")
		);
		SendDlgItemMessageA(
			hMainWnd, IDC_CLOCK, WM_SETFONT,
			(WPARAM)fontHandle, TRUE
		);
		SendDlgItemMessageA(
			hMainWnd, IDC_PAUSE, BM_SETIMAGE,
			IMAGE_BITMAP, (LPARAM)pausePic
		);

		hClockText = GetDlgItem(hMainWnd, IDC_CLOCK);

		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_PAUSE:
			
			if (!ClockPaused) {

				SuspendThread(hClockThread);
				ClockPaused = true;

				SendDlgItemMessageA(
					hMainWnd, IDC_PAUSE, BM_SETIMAGE,
					IMAGE_BITMAP, (LPARAM)playPic
				);
			}
			else {

				ResumeThread(hClockThread);
				ClockPaused = false;

				SendDlgItemMessageA(
					hMainWnd, IDC_PAUSE, BM_SETIMAGE,
					IMAGE_BITMAP, (LPARAM)pausePic
				);
			}
			InvalidateRect(hClockText, NULL, true);
			return true;

		case IDC_BROWSE:
		{
			char fileName[MAX_PATH] = "notepad.exe";
			if (BrowseFileName(hWnd, fileName)) {
				SetDlgItemText(hWnd, IDC_COMMANDLINE, fileName);
			}
			
			return TRUE;
		}

		case IDC_ADD:
		{
			SYSTEMTIME plannedTime;
			SendDlgItemMessageA(
				hMainWnd, IDC_TIME, DTM_GETSYSTEMTIME,
				0, (LPARAM)&plannedTime
			);
			int plannedTimeI = SystemTimeToInt(plannedTime);

			SYSTEMTIME currentTime;
			GetLocalTime(&currentTime);
			int currentTimeI = SystemTimeToInt(currentTime);

			if (plannedTimeI < currentTimeI) return TRUE;

			CloseHandle(CreateThread(NULL, 0, ScheduleThread, NULL, 0, NULL));

			return TRUE;
		}
		case IDC_DELETE:
		{
			LRESULT selectedLineIndex = SendDlgItemMessageA(
				hMainWnd, IDC_TIMELIST, LB_GETCURSEL,
				0, 0
			);
			if (selectedLineIndex == LB_ERR) return TRUE;

			SendDlgItemMessageA(
				hMainWnd, IDC_TIMELIST, LB_DELETESTRING,
				selectedLineIndex, 0
			);

			return TRUE;
		}
		case IDOK:
			DestroyWindow(hWnd);
			return TRUE;

		}
		return FALSE;

	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;
		int dlgItemId = GetDlgCtrlID((HWND)lParam);

		if (dlgItemId == IDC_CLOCK) 
		{
			if (ClockPaused == false) {
				SetTextColor(hdcStatic, RGB(0, 0, 255));
			}
			else {
				SetTextColor(hdcStatic, RGB(255, 0, 0));
			}
		}

		if (hbrBkgnd == NULL)
		{
			hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
		}
		return (INT_PTR)hbrBkgnd;
	}

	case WM_DESTROY:
		
		Terminate = true;
		Sleep(500);
		CloseHandle(hClockThread);
		DeleteObject(fontHandle);
		DeleteObject(hbrBkgnd);
		
		PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}