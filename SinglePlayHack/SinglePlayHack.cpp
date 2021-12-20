#include <iostream>
#include <string>
#include <Windows.h>
#include <tchar.h>
#include <Psapi.h>
#include <TlHelp32.h>
using namespace std;

#define PLAYER_BASE_OFFSET 0x10F4F4
#define PLAYER_RIFFLE_BULLET_OFFSET 0x150
#define PLAYER_HEALTH_OFFSET 0xF8
#define PLAYER_GRENADE_OFFSET 0x158
#define PLAYER_COUNT_OFFSET 0x10F500

/*
	Name:
		GetPID
	Purpose:
		ac_client.exe 의 PID 를 얻어옴.
	Args:
		procName : 프로세스 이름
*/
DWORD GetPID(const wchar_t* procName) {
	DWORD pid = 0;
	// 현재 프로세스에 대한 snapshot 가져옴
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		cout << "프로세스 목록을 가져올 수 없습니다\n";
		return 0;
	}
	/* procEntry 구조체 선언 */
	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(procEntry);
	
	if (Process32First(hSnapshot, &procEntry)) {
		do {
			if (!wcscmp(procEntry.szExeFile, procName)) {
				/* ac_client.exe 걸러내기 */
				pid = procEntry.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &procEntry));
	}
	return pid;
}

/*
	Name:
		GetModuleBaseAddress
	Purpose:
		
	Args:
		pid : ac_client.exe process ID
		modName : ac_client.exe 프로세스 이름
*/
DWORD GetModuleBaseAddress(DWORD pid, const wchar_t *modName) {
	DWORD ModBaseAddr = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	// Get modules snapshot of pid
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 ModEntry;
		ModEntry.dwSize = sizeof(ModEntry);

		if (Module32First(hSnapshot, &ModEntry)) {
			// First Module of Process is "Process File Name"
			do {
				if (!_wcsicmp(ModEntry.szModule, modName)) {
					ModBaseAddr = (DWORD)ModEntry.modBaseAddr;
					// In 32bit, Equal to Image Base
					break;
				}
			} while (Module32Next(hSnapshot, &ModEntry));
		}
	}
	CloseHandle(hSnapshot);
	return ModBaseAddr;
}

/*
	Name:
		WriteValue
	Purpose:
		원하는 메모리에 원하는 값을 쓴다.
	Args:
		hProc : ac_client.exe 프로세스
		addr : 값을 쓸 메모리
		changeValue : 바꿀 값
		criteria : 값을 바꿀 기준 값
*/
void WriteValue(HANDLE hProc, DWORD addr, DWORD changeValue, DWORD criteria) {
	DWORD currentValue;
	if (!ReadProcessMemory(hProc, (BYTE *)addr, &currentValue, sizeof(currentValue), 0)) {
		cout << "[Error] 메모리를 읽을 수 없습니다\n";
		exit(0);
	}
	if (currentValue < criteria)
		WriteProcessMemory(hProc, (BYTE *)addr, &changeValue, sizeof(changeValue), 0);
}

int main() {
	// 플레이어 오브젝트 주소가 담길 곳
	DWORD PlayerInfo, PlayerCount;
	DWORD pid = GetPID(L"ac_client.exe");
	DWORD moduleBaseAddr = GetModuleBaseAddress(pid, L"ac_client.exe");
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,0,pid);
	if (!hProcess) {
		cout << "[Error] Assault Cube 의 process 를 가져올 수 없습니다\n";
		return 0;
	}
	ReadProcessMemory(hProcess, (BYTE*)moduleBaseAddr + PLAYER_BASE_OFFSET, &PlayerInfo, sizeof(PlayerInfo), NULL);
	ReadProcessMemory(hProcess, (BYTE*)moduleBaseAddr + PLAYER_COUNT_OFFSET, &PlayerCount, sizeof(PlayerCount), NULL);
	DWORD health_ptr = PlayerInfo + PLAYER_HEALTH_OFFSET;
	DWORD riffle_bullet_ptr = PlayerInfo + PLAYER_RIFFLE_BULLET_OFFSET;
	DWORD grenade_ptr = PlayerInfo + PLAYER_GRENADE_OFFSET;
	cout << PlayerCount << "\n";
	while (1) {
		WriteValue(hProcess, health_ptr, 100, 99); // 체력무한
		WriteValue(hProcess, riffle_bullet_ptr, 20, 10); // 총알무한(라이플)
		WriteValue(hProcess, grenade_ptr, 3, 1); // 수류탄 무한
	}
}