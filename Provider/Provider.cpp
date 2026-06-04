#include <Windows.h>
#include <iostream>
#include "myprovider.h"

#pragma comment(lib, "advapi32.lib")

//static const GUID my_guid =
//{ 0x6a282996,0x820b,0x42f3,{0x87,0x25,0x57,0x3f,0xd3,0x09,0xcd,0x0a} };

REGHANDLE g_EtwHandle = 0;

int main()
{
    ULONG error;

    std::cout << "[+] Iniciando Provider\n";

    // Registrar usando macro generada por mc
    error = EventRegisterMyCustomETWProvider(&g_EtwHandle);

    if (error != ERROR_SUCCESS)
    {
        std::cout << "[-] EventRegister error: " << error << "\n";
        return -1;
    }

    std::cout << "[+] Provider registrado\n";

    DWORD pid = GetCurrentProcessId();
    WCHAR text[] = L"Hola Mundo";

    // Llamada generada automáticamente por mc
    error = EventWriteProcessMessage(
        pid,
        text
    );

    if (error != ERROR_SUCCESS)
    {
        std::cout << "[-] EventWrite error: " << error << "\n";
    }
    else
    {
        std::cout << "[+] Evento enviado correctamente\n";
    }

    std::cout << "[+] Presiona Enter para salir...\n";
    std::cin.get();

    EventUnregister(g_EtwHandle);

    return 0;
}