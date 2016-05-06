// DbgOut.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WinUtils.h"

HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
WORD wAttributes = 0;
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    if (wAttributes != 0)
        SetConsoleTextAttribute(hStdOut, wAttributes);
    return FALSE;
}

struct DbgData
{
    DWORD   dwProcessId;
    char    data[4096 - sizeof(DWORD)];
};

int _tmain(int argc, _TCHAR* argv[])
{
	bool bDisplayABout = false;

	for (int arg = 1; arg < argc; ++arg)
	{
		if (_tcscmp(argv[arg], L"/?") == 0)
			bDisplayABout = true;
	}

	if (bDisplayABout)
	{
		DisplayAboutMessage(NULL);
		return 0;
	}

    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi = {};
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    wAttributes = csbi.wAttributes;
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

#if 0
    LPCTSTR DBWinMutex = L"DBWinMutex";
    HandlePtr hDBWinMutex(::OpenMutex(MUTEX_ALL_ACCESS, FALSE, DBWinMutex));

    if (!hDBWinMutex)
    {
        DWORD errorCode = GetLastError();
        _ftprintf(stderr, L"Error opening %s: %d\n", DBWinMutex, errorCode);
        return errorCode;
    }
#endif

    LPCTSTR DBWIN_BUFFER_READY = L"DBWIN_BUFFER_READY";
    HandlePtr hEventBufferReady(::OpenEvent(EVENT_ALL_ACCESS, FALSE, DBWIN_BUFFER_READY));

    if (!hEventBufferReady)
    {
        hEventBufferReady.reset(CreateEvent(NULL, FALSE, TRUE, DBWIN_BUFFER_READY));   // auto-reset, initial state: signaled

        if (!hEventBufferReady)
        {
            DWORD errorCode = GetLastError();
            _ftprintf(stderr, L"Error opening %s: %d\n", DBWIN_BUFFER_READY, errorCode);
            return errorCode;
        }
    }

    LPCTSTR DBWIN_DATA_READY = L"DBWIN_DATA_READY";
    HandlePtr hEventDataReady(::OpenEvent(SYNCHRONIZE, FALSE, DBWIN_DATA_READY));

    if (!hEventDataReady)
    {
        hEventDataReady.reset(::CreateEvent(NULL, FALSE, FALSE, DBWIN_DATA_READY));  // auto-reset, initial state: nonsignaled

        if (!hEventDataReady)
        {
            DWORD errorCode = GetLastError();
            _ftprintf(stderr, L"Error opening %s: %d\n", DBWIN_DATA_READY, errorCode);
            return errorCode;
        }
    }

    LPCTSTR DBWIN_BUFFER = L"DBWIN_BUFFER";
    HandlePtr hDBMonBuffer(::OpenFileMapping(FILE_MAP_READ, FALSE, DBWIN_BUFFER));

    if (!hDBMonBuffer)
    {
        hDBMonBuffer.reset(CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DbgData), DBWIN_BUFFER));

        if (!hDBMonBuffer)
        {
            DWORD errorCode = GetLastError();
            _ftprintf(stderr, L"Error opening %s: %d\n", DBWIN_BUFFER, errorCode);
            return errorCode;
        }
    }

    DbgData* pDBBuffer = (DbgData*) ::MapViewOfFile(hDBMonBuffer.get(), SECTION_MAP_READ, 0, 0, 0);

    if (!pDBBuffer)
    {
        DWORD errorCode = GetLastError();
        _ftprintf(stderr, L"Error MapViewOfFile: %d\n", errorCode);
        return errorCode;
    }

    WORD Colors[] = {
        FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        FOREGROUND_RED | FOREGROUND_INTENSITY,
        FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY,
        FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
        FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
    };
    WORD bgColor = csbi.wAttributes & (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
    while (true)
    {
        DWORD ret = ::WaitForSingleObject(hEventDataReady.get(), INFINITE);

        if (ret == WAIT_OBJECT_0)
        {
            SYSTEMTIME rTimeDate;
            GetLocalTime(&rTimeDate);
            wchar_t strDate[256] = L"";
            GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &rTimeDate, NULL, strDate, ARRAYSIZE(strDate));
            wchar_t strTime[256] = L"";
            GetTimeFormat(LOCALE_USER_DEFAULT, 0, &rTimeDate, NULL, strTime, ARRAYSIZE(strTime));

            wchar_t strProcessName[MAX_PATH] = L"";
            HandlePtr hProcess(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pDBBuffer->dwProcessId));
            if (hProcess)
            {
                GetProcessImageFileName(hProcess.get(), strProcessName, ARRAYSIZE(strProcessName));
            }

            SetConsoleTextAttribute(hStdOut, bgColor | Colors[pDBBuffer->dwProcessId % ARRAYSIZE(Colors)]);
            LPCTSTR strExeName = _tcsrchr(strProcessName, '\\');
            strExeName = strExeName ? strExeName + 1 : strProcessName;
            wprintf(L"%s %s %d %s ", strDate, strTime, pDBBuffer->dwProcessId, strExeName);
            printf(pDBBuffer->data);
            SetEvent(hEventBufferReady.get());
        }
    }

    SetConsoleTextAttribute(hStdOut, csbi.wAttributes);

    if (pDBBuffer != NULL)
    {
        ::UnmapViewOfFile(pDBBuffer);
        pDBBuffer = NULL;
    }

    return 0;
}
