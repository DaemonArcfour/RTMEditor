#define _CRT_SECURE_NO_WARNINGS
#include "RTM_Def.h"
struct LoopIntArgs {
	int val;
	DWORD Address;
};
struct LoopFloatArgs {
	float val;
	DWORD Address;
};

void LoopInt(LPVOID pArgs) {
	LoopIntArgs *pArguments = (LoopIntArgs*)pArgs;	// Get the value that we want to force
	static int *val = new int(pArguments->val);
	static DWORD *Address = new DWORD(pArguments->Address); // Address where it is going to be forced
	while (true) {
		StaticPatcher::SetInfo::AssignIntegerToAddress(*val, *Address); // Set it in a loop
		Sleep(10);
	}
}

void LoopFloat(LPVOID pArgs) {
	LoopFloatArgs *pArguments = (LoopFloatArgs*)pArgs;
	static float *local_val = new float(pArguments->val);
	static DWORD *local_address = new DWORD(pArguments->Address);
	while (true) {
		StaticPatcher::SetInfo::AssignFloatToAddress(*local_val, *local_address);
		Sleep(10);
	}
}

namespace StaticPatcher {

	int InfoGathering::GetIntegerFromAddress(DWORD Address) {
		int *pInt = reinterpret_cast<int*>(Address);
		return *pInt;
	}
	float InfoGathering::GetFloatFromAddress(DWORD Address) {
		float *pFloat = reinterpret_cast<float*>(Address);
		return *pFloat;
	}
	char* InfoGathering::GetStringFromAddress(DWORD Address) {
		char* pChar = new char[512];
		pChar = reinterpret_cast<char *>(Address);
		return pChar;
	}
	void SetInfo::AssignIntegerToAddress(int val, DWORD Address) {
		int *pInt = (int*)Address;
		*pInt = val;
	}
	void SetInfo::AssignFloatToAddress(float val, DWORD Address) {
		float *pFloat = (float*)Address;
		*pFloat = val;
	}
	void SetInfo::AssignStringToAddress(char* str, DWORD Address) {
		char *pChar = (char*)Address;
		pChar = str;
	}
	void Patching::WriteBytes(BYTE Bytes[], DWORD Address, int size) {
		DWORD escape = 0x0;
		for (int i = 0; i < size; i++)
		{
			DWORD dwProtect = 0x0;
			VirtualProtect((LPVOID)(Address + escape), 1, PAGE_EXECUTE_READWRITE, &dwProtect);
			BYTE *ptr = (BYTE *)Address + escape;
			*ptr = Bytes[i];
			VirtualProtect((LPVOID)(Address + escape), 1, dwProtect, NULL);
			escape += 0x1;
		}
	}
	void *Patching::DetourFunction(BYTE *Source, const BYTE *Destination, const int Length)
	{
		BYTE *jmp = (BYTE*)malloc(Length + 5);
		DWORD dwback;
		VirtualProtect(Source, Length, PAGE_READWRITE, &dwback);
		memcpy(jmp, Source, Length);
		jmp += Length;
		jmp[0] = 0xE9;
		*(DWORD*)(jmp + 1) = (DWORD)(Source + Length - jmp) - 5;
		Source[0] = 0xE9;
		*(DWORD*)(Source + 1) = (DWORD)(Destination - Source) - 5;
		VirtualProtect(Source, Length, dwback, &dwback);
		return (jmp - Length);
	}
	void Patching::CreateWarp(BYTE *Address, DWORD WarpTo, DWORD Length)
	{
		DWORD dwOldProtect, dwBkup, dwRelAddr;
		VirtualProtect(Address, Length, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		dwRelAddr = (DWORD)(WarpTo - (DWORD)Address) - 5;
		*Address = 0xE9;
		*((DWORD *)(Address + 0x1)) = dwRelAddr;
		for (DWORD x = 0x5; x < Length; x++) *(Address + x) = 0x90;
		VirtualProtect(Address, Length, dwOldProtect, &dwBkup);
		return;
	}
	bool SearchOffset::Compare(const BYTE* pData, const BYTE* bMask, const char* szMask)
	{
		for (; *szMask; ++szMask, ++pData, ++bMask)
			if (*szMask == 'x' && *pData != *bMask)   return 0;
		return (*szMask) == NULL;
	}

	DWORD SearchOffset::FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char * szMask)
	{
		for (DWORD i = 0; i < dwLen; i++) {
			if (Compare((BYTE*)(dwAddress + i), bMask, szMask))  
				return (DWORD)(dwAddress + i);

			#ifndef _DBG
				printf("[%x] ", dwAddress + i);
			#endif
		}

		return 0;
	}

	void Freeze::FreezeInteger(int val, DWORD Address) {
		LoopIntArgs pArgs = { val, Address };
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)LoopInt, &pArgs, 0, 0);
	}
	void Freeze::FreezeFloat(float val, DWORD Address) {
		LoopFloatArgs pArgs = { val, Address };
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)LoopFloat, &pArgs, 0, 0);
	}
	void Miscellaneous::OpenConsole() {
		AllocConsole();
		freopen("conin$", "r", stdin);
		freopen("conout$", "w", stdout);
		freopen("conout$", "w", stderr);
	}
}