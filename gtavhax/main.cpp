
#include "stdafx.h"
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <WinInet.h>
#pragma comment( lib, "wininet" )
#include "time.h"

// Some of this is from/based on example code from MSDN

DWORD FindProc(TCHAR* peName);
int EarnMoney(DWORD pid);
void printError(TCHAR* msg);

int main(void)
{
#ifdef PUBREL
	_tprintf(TEXT("GTA V: Online Money Hack\n===========================================================\n\n"));
	Sleep(10000);
#endif
	int totalEarned = 0;
	do {
		DWORD pid;
		srand(time(NULL));
		while (!(pid = FindProc(TEXT("GTA5.exe")))) {
			Sleep(5000);
		}
		_tprintf(TEXT("\n  PID: %d"), pid);
		totalEarned += EarnMoney(pid);

		int delay = (rand() % (45000 - 30000)) + 30000;

		_tprintf(TEXT("\n > Total earned: %d$. Sleeping %dms...\n"), totalEarned, delay);
		Sleep(delay);

	} while (TRUE);
	return 0;
}


DWORD FindProc(TCHAR* peName) {
	_tprintf(TEXT("\nSearching for: %s"), peName);

	DWORD ret = 0;
	HANDLE hProcessSnap;

	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printError(TEXT("CreateToolhelp32Snapshot (of processes)"));
		return(FALSE);
	}
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap, &pe32))
	{
		printError(TEXT("Process32First")); // show cause of failure
	}
	else {
		do
		{
			if (_tcscmp(pe32.szExeFile, peName) == 0) {
				ret = pe32.th32ProcessID;
			}
		} while (!ret && Process32Next(hProcessSnap, &pe32));

	}
	CloseHandle(hProcessSnap);

	return ret;
}



int EarnMoney(DWORD pid)
{
	int	amount = 0;
	HANDLE hProcess;
	// Retrieve the priority class.
	DWORD dwPriorityClass = 0;
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL)
		printError(TEXT("OpenProcess"));
	else
	{
		dwPriorityClass = GetPriorityClass(hProcess);
		if (!dwPriorityClass) {
			printError(TEXT("GetPriorityClass"));
		} else {

			///////////////// MODULE STUFF ////////////////////////
			HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
			MODULEENTRY32 me32;

			//  Take a snapshot of all modules in the specified process. 
			hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
			if (hModuleSnap == INVALID_HANDLE_VALUE)
			{
				printError(TEXT("CreateToolhelp32Snapshot (of modules)"));
			}

			//  Set the size of the structure before using it. 
			me32.dwSize = sizeof(MODULEENTRY32);

			//  Retrieve information about the first module, 
			//  and exit if unsuccessful 
			if (!Module32First(hModuleSnap, &me32))
			{
				printError(TEXT("Module32First"));  // Show cause of failure 
				CloseHandle(hModuleSnap);     // Must clean up the snapshot object! 
			}

			//  Now walk the module list of the process, 
			//  and display information about each module 

			do
			{
				if (_tcscmp(me32.szModule, TEXT("GTA5.exe")) == 0) {
					_tprintf(TEXT("\n\n  MODULE NAME:     %s"), me32.szModule);
					_tprintf(TEXT("\n  executable     = %s"), me32.szExePath);
					_tprintf(TEXT("\n  process ID     = 0x%08X"), me32.th32ProcessID);
					_tprintf(TEXT("\n  base address   = 0x%016llX"), me32.modBaseAddr);
					_tprintf(TEXT("\n  base size      = %d"), me32.modBaseSize);


					// handle/username? me32.modBaseAddr + 0x2954234
					// prod.*.pod.rockstargames.com?  + 0x29532AC
					//void* hostPtr = (void*)(me32.modBaseAddr + 0x29532AC)
					
					//void* hostPtr = (void*)(me32.modBaseAddr + 0x29651F0); // sometimes telemetry sometimes pod?
					void* hostPtr = (void*)(me32.modBaseAddr + 0x29746AC); // VER_1_0_335_2_STEAM
					char host[64] = { 0 };
					//+ 0x9a14340 - 0x5010 = 0x9A0F330
					ReadProcessMemory(hProcess, hostPtr, host, sizeof(host), NULL);
					_tprintf(TEXT("\n  host: %016llX -> %hs"), hostPtr, host);


					// Pointer
					// void* bufPtrPtr = (void*)(me32.modBaseAddr + 0x01DCCC08);
					void* bufPtrPtr = (void*)(me32.modBaseAddr + 0x01DD1E28);  // VER_1_0_335_2_STEAM
					unsigned long long bufPtrValue;
					ReadProcessMemory(hProcess, bufPtrPtr, &bufPtrValue, sizeof(bufPtrValue), NULL);
					_tprintf(TEXT("\n  PTR: %016llX -> %016llx"), bufPtrPtr, bufPtrValue);

					// deref+0xc0 = gsToken
					char gsToken[42] = { 0 };
					void* gsTokenPtr = (void*)(bufPtrValue + 0xc0);
					ReadProcessMemory(hProcess, (void*)gsTokenPtr, gsToken, sizeof(gsToken), NULL);
					_tprintf(TEXT("\n  gsToken: %016llX -> %hs"), gsTokenPtr, gsToken);


					// deref+ 0x100
					void* txnNonceSeedPtr = (void*)(bufPtrValue + 0x100);
					unsigned long txnNonceSeed;
					ReadProcessMemory(hProcess, txnNonceSeedPtr, &txnNonceSeed, sizeof(txnNonceSeed), NULL);
					_tprintf(TEXT("\n  txnSeed: %016llX -> %d"), txnNonceSeedPtr, txnNonceSeed);

					// deref+ 0x130
					void*  txnNoncePtr = (void*)(bufPtrValue + 0x190);
					unsigned long txnNonce = 0;
					ReadProcessMemory(hProcess, txnNoncePtr, &txnNonce, sizeof(txnNonce), NULL);
					_tprintf(TEXT("\n      txn: %016llX -> %d"), txnNoncePtr, txnNonce);

					if (txnNonce != 0 && txnNonce >= txnNonceSeed && txnNonce-txnNonceSeed <= 256) { // 256 is arbitrary... precautionary, wouldn't want too many txns before an Update occurs?

						TCHAR hdrs[127] = { 0 };
						wsprintf(hdrs, TEXT("Content-Type: application/text\r\nAuthorization: GSTOKEN token=%hs\r\n"), gsToken);
						_tprintf(TEXT("\n%s"), hdrs);

						char json[255] = { 0 };
						int slot = 0;

						long long options[][3] = {
#ifdef PUBREL
							{ -46622315, 133700, 133799 }, // SELL A CAR
#else
							{ -31156877, 1, 80, }, // PEDESTRIAN
							{ -1027218631, 480, 630}, // RACE
							{ -1127021384, 2000, 2001 }, // GOOD PLAYER
							{ -1398318418, 100, 9000 }, // RACE BETS
							{ -46622315, 800, 26000 }, // SELL A CAR
							{ -31156877, 200, 10000 }, // PLAYER
							{ -1985150258, 100, 20000 }, // SHARE CASH FROM LAST JOB (RECIEVE)
							{ -1027218631, 480, 7000 }, // MUGGER
							{ 393059668, 36000, 140000 }, // HEIST
							{ -1586170317, 100000, 100001 }, // HEIST BONUS
							{ 1734805203, 1200, 1800 }, // ROB A STORE
							{ 1734805203, 200, 250 }, // ROB A STORE
#endif
						};

						int n = rand() % sizeof(options)/sizeof(options[0]);

						int lowerBound = options[n][1];
						int upperBound = options[n][2];
						amount = (rand() % (upperBound - lowerBound)) + lowerBound;
						signed long long itemId = options[n][0];

						sprintf_s(json, "{\"catalogVersion\":21,\"TransactionNonce\":%d,\"slot\":%d,\"items\":[{\"itemId\":%d,\"value\":1,\"price\":%d}],\"bank\":0,\"wallet\":%d}", txnNonce, slot, itemId, amount, amount);

						TCHAR hostw[64] = { 0 };
						wsprintf(hostw, TEXT("%hs"), host);
						TCHAR url[] = TEXT("/gta5/11/GamePlayServices/GameTransactions.asmx/Earn");

						//*
						_tprintf(TEXT("\n > EARNING %d$ for itemId: %d"), amount, itemId);
						_tprintf(TEXT("\n\n  Host: %s"), hostw);
						_tprintf(TEXT("\n  URL: %s"), url);
						_tprintf(TEXT("\n  %hs"), json);
						//*/

						HINTERNET hSession = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
						HINTERNET hConnect = InternetConnect(hSession, hostw, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
						HINTERNET hRequest = HttpOpenRequest(hConnect, TEXT("POST"), url, NULL, NULL, NULL, INTERNET_FLAG_SECURE, 1);
						HttpSendRequest(hRequest, hdrs, wcslen(hdrs), json, strlen(json));
						// TODO: close connection handle


						//ReadProcessMemory(hProcess, txnNoncePtr, &txnNonce, sizeof(txnNonce), NULL);
						txnNonce += 1;
						WriteProcessMemory(hProcess, txnNoncePtr, &txnNonce, sizeof(txnNonce), NULL);
						_tprintf(TEXT("\n  incrementing txnNonce: %016llX -> %d (+1)"), txnNoncePtr, txnNonce);
					}
					else {
						_tprintf(TEXT("\n Invalid txn/seed pair. Try joing a new session, and waiting (up to five minutes)."));
					}



				}

			} while (Module32Next(hModuleSnap, &me32));

			CloseHandle(hModuleSnap);
			//////////////////// END MODULE STUFF //////////////////////////////////////////////


			CloseHandle(hProcess);
		}
	}
	return amount;

}

void printError(TCHAR* msg)
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, eNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		sysMsg, 256, NULL);

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while ((*p > 31) || (*p == 9))
		++p;
	do { *p-- = 0; } while ((p >= sysMsg) &&
		((*p == '.') || (*p < 33)));

	// Display the message
	_tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}



