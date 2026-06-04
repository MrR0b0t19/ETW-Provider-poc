#include <Windows.h>
#include <Evntrace.h>
#include <Evntcons.h>
#include <iostream>

#pragma comment(lib, "advapi32.lib")

// Mismo GUID que tu provider
static const GUID MY_GUID =
{ 0x6a282996,0x820b,0x42f3,{0x87,0x25,0x57,0x3f,0xd3,0x09,0xcd,0x0a} };

#define SESSION_NAME L"MyEtwSession"

// Callback cuando llega un evento
void WINAPI EventRecordCallback(EVENT_RECORD* record)
{
    if (!record)
        return;

    std::wcout << L"\n[+] Evento recibido\n";

    // Verificar ID
    std::wcout << L"    EventID: "
        << record->EventHeader.EventDescriptor.Id
        << L"\n";

    BYTE* ptr = (BYTE*)record->UserData;

    // Primer campo: PID
    DWORD pid = *(DWORD*)ptr;
    ptr += sizeof(DWORD);

    // Segundo campo: string
    WCHAR* text = (WCHAR*)ptr;

    std::wcout << L"    PID: " << pid << L"\n";
    std::wcout << L"    Msg: " << text << L"\n";
}

int main()
{
    ULONG status;
    EVENT_TRACE_PROPERTIES* props;
    TRACEHANDLE sessionHandle = 0;
    TRACEHANDLE traceHandle = 0;

    // Reservar memoria para propiedades
    ULONG bufferSize =
        sizeof(EVENT_TRACE_PROPERTIES) +
        sizeof(SESSION_NAME);

    props = (EVENT_TRACE_PROPERTIES*)malloc(bufferSize);
    ZeroMemory(props, bufferSize);

    props->Wnode.BufferSize = bufferSize;
    props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    props->Wnode.ClientContext = 1; // QPC timer
    props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

    props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    // Crear sesión
    status = StartTrace(
        &sessionHandle,
        SESSION_NAME,
        props
    );

    if (status != ERROR_SUCCESS)
    {
        std::cout << "StartTrace failed: " << status << "\n";
        return -1;
    }

    std::cout << "[+] Session started\n";

    // Habilitar tu provider
    status = EnableTraceEx2(
        sessionHandle,
        &MY_GUID,
        EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_VERBOSE,
        0,
        0,
        0,
        NULL
    );

    if (status != ERROR_SUCCESS)
    {
        std::cout << "EnableTraceEx2 failed: " << status << "\n";
        return -1;
    }

    std::cout << "[+] Provider enabled\n";

    // Configurar consumo
    EVENT_TRACE_LOGFILE trace = { 0 };

    trace.LoggerName = (LPWSTR)SESSION_NAME;
    trace.ProcessTraceMode =
        PROCESS_TRACE_MODE_REAL_TIME |
        PROCESS_TRACE_MODE_EVENT_RECORD;

    trace.EventRecordCallback = EventRecordCallback;

    traceHandle = OpenTrace(&trace);

    if (traceHandle == INVALID_PROCESSTRACE_HANDLE)
    {
        std::cout << "OpenTrace failed\n";
        return -1;
    }

    std::cout << "[+] Listening...\n";

    // Bucle principal (bloqueante)
    status = ProcessTrace(
        &traceHandle,
        1,
        NULL,
        NULL
    );

    std::cout << "ProcessTrace ended: " << status << "\n";

    // Cerrar
    CloseTrace(traceHandle);
    ControlTrace(sessionHandle, SESSION_NAME, props, EVENT_TRACE_CONTROL_STOP);

    free(props);

    return 0;
}
