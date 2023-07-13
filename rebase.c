/*
Rebase.c by alsch092 @ github
Concept snippet for executing an image in a different/copied address space
*/

#include <iostream>
#include <Windows.h>
#include <Psapi.h>

int main();

void ChainedTestFunction() 
{
	printf("Return address (ChainedTestFunction): %x\n", *(DWORD*)_AddressOfReturnAddress());
}

void TestFunction() //demonstrates how address context can change while still executing correctly
{
	printf("Return address (TestFunction): %x\n", *(DWORD*)_AddressOfReturnAddress());
	ChainedTestFunction();
}

void SecretFunction() //this routine will only be called by the copy of our image base, making the results of static analyzing depective as they imply the original imagebase is calling this.
{
	printf("Return address (SecretFunction): %x\n", *(DWORD*)_AddressOfReturnAddress());
}

// Function to copy image memory and resume execution
static bool RebaseImage()
{
	static bool rebased = false; //our basic 'switch', turns on once we've rebased which stops future rebasing

	if (rebased)
		return false;

	DWORD dwOldProt = 0;
	HANDLE hProcess = GetCurrentProcess();

	HMODULE hModule = GetModuleHandle(NULL);
	LPVOID baseAddress = hModule;
	DWORD dwProt = 0;
	DWORD threadId = 0;

	//Get the image size
	MODULEINFO moduleInfo;
	GetModuleInformation(hProcess, hModule, &moduleInfo, sizeof(MODULEINFO));
	SIZE_T imageSize = moduleInfo.SizeOfImage;

	//Allocate memory for the new image
	LPVOID newImageAddress = VirtualAlloc(NULL, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (newImageAddress != NULL)
	{
		//Update the image base address!
		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)newImageAddress;
		PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)newImageAddress + pDosHeader->e_lfanew);
		pNtHeaders->OptionalHeader.ImageBase = (DWORD_PTR)newImageAddress; //modify ImageBase address (not offset!) in Optional Header to reflect our new imagebase. todo: check if this is even needed

		//Copy the image memory to the new location
		memcpy(newImageAddress, baseAddress, imageSize);

		DWORD mainFuncOffset = (DWORD)main - (DWORD)moduleInfo.lpBaseOfDll; //Get offset of our main routine

		DWORD rebased_main = (DWORD)(newImageAddress) + mainFuncOffset;

		rebased = true; //turn switch ON

		HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)rebased_main, NULL, 0, &threadId); //now we create a new thread to resume execution at the new image location or some custom spot
		
		if (hThread != NULL)
		{
			ExitThread(0); //exit current thread (not hThread), resume execution at main() 
		}	
	}
	else
	{
		std::cerr << "Failed to allocate memory for the new image." << std::endl;
		return false;
	}
	
	rebased = true;
	return true;
}

/*
The goal of this program is to create a copy of the image at some other address and then change running context (via a jump, or new thread) to resume execution inside the image copy.
*/
int main()
{
	TestFunction();

	printf("Original Imagebase: %x\n", (DWORD)GetModuleHandle(L"RebaseImage.exe"));
	
	if (RebaseImage())  //this routine is actually called twice, once from the original image main() and once from copy_image's main()
	{
		printf("Rebased image to new address. New Imagebase: %x\n", (DWORD)GetModuleHandle(L"RebaseImage.exe"));
	}
	else //the second time calling RebaseImage will fail due to 'rebased' being set to true
	{
		printf("Either rebase failed or we are running in new base context/address!\n");
	}

	TestFunction(); //compare the return address to the same call above, it should be in a different 'module' now

	SecretFunction(); //this will only be called by the copy of the image, not the original image. by the time the image copy reaches here, it's calling the rebased address of TestFunction

	MessageBoxA(0, "Check address of where this is called from", 0, 0);

	TerminateProcess(GetCurrentProcess(), 0); //we need to call this ourselves, exiting the thread naturally seems to run into some error in ZwTerminateThread, the program will hang because the main thread was exited before, some sort of bookkeeping is going wrong under the hood
	
	return 0; //this won't be reached normally
}
