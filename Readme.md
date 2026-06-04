# Agrego este readme para no olvidar cosas importantes
Lo primero es que debemos entender que nuestro <b>Provider</b> es nuestro proveedor de eventos o aquel que los despliega (surgen varias dudas pero primero quiero profundizar en conceptos), existen dos maneras de crearlo:

## 1. Manera Manual

La primera es la más simple y no es mapeado por Windows, simplemente es creado en código:

```Cpp

#include <Windows.h>
#include <iostream>
#include <evntprov.h>

#pragma comment(lib, "advapi32.lib")

REGHANDLE g_EtwHandle = 0;

// Descriptor del evento
EVENT_DESCRIPTOR g_EventRestar =
{
    1,    // Id
    0,    // Version
    0,    // Channel
    4,    // Level (Information)
    0,    // Opcode
    0,    // Task
    0     // Keyword
};

// GUID del provider
static const GUID my_guid =
{ 0x6a282996,0x820b,0x42f3,{0x87,0x25,0x57,0x3f,0xd3,0x09,0xcd,0x0a} };

int main()
{
    ULONG error;

    std::cout << "[+] Iniciando Provider\n";

    error = EventRegister(&my_guid, NULL, NULL, &g_EtwHandle);

    if (error != ERROR_SUCCESS)
    {
        std::cout << "[-] EventRegister error: " << error << "\n";
        return -1;
    }

    std::cout << "[+] Provider registrado\n";

    DWORD pid = GetCurrentProcessId();

    WCHAR text[] = L"Enviado desde el provider";

    EVENT_DATA_DESCRIPTOR data[2];

    EventDataDescCreate(
        &data[0],
        &pid,
        sizeof(pid)
    );

    EventDataDescCreate(
        &data[1],
        text,
        (ULONG)((wcslen(text) + 1) * sizeof(WCHAR))
    );

    error = EventWrite(
        g_EtwHandle,
        &g_EventRestar,
        2,
        data
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

    error = EventUnregister(g_EtwHandle);

    if (error != ERROR_SUCCESS)
    {
        std::cout << "[-] EventUnregister error: " << error << "\n";
    }

    return 0;
}

```

El anterior código es el más sencillo, generamos aquí mismo el Provider y ocupamos un consumidor que obtenga sus propios logs, no se encuentra registrado en el sistema y simplemente es un proveedor que genera mensajes en crudo.

Los detalles del anterior código (esto es empírico) es que no los mapea o tracea (identifica, reconoce, etc.) Windows o algún otro programa que detecte a los proveedores que se tienen trabajando en el sistema operativo.

Puedo decir que me causó problemas entender un poco esto, pero es cómo enviar un mensaje que puede llegarle a cualquiera más no se va a tracear, simplemente es enviarlo al ETW y puede llegarle a algún consumidor, pero en este caso, es nuestro consumidor en el ejemplo de [Consumidor](Consumidor/Consumer.cpp) al que le llegara el LOG.

> IMPORTANTE \
> Los GUID siempre deben coincidir entre ```Provider``` y ```Consumer```

## 2. Manera Registrada

### Archivo Manifiesto
Para la manera registrada en windows solo ocupamos generar un archivo ```.man``` (archivo de manifiesto). Yo veo este archivo cómo la descripción de nuestro ```Provider```, contiene información sobre los eventos, la DLL que se utiliza para entender los datos (Este es importante para ver cómo son los datos), tipo de dato que lleva cada evento y tareas (tasks).

El archivo lo podemos ver en la siguiente dirección [myprovider.man](Provider/myprovider.man)

Podemos generar todos los documentos necesarios para trabajar con el usando ```mc.exe```, es un programa que nos ayuda a compilar nuestro manifiesto y nos generara un archivo de cabecera header (```.h```) el cual podemos incluir en nuestro programa para enviar los logs a nuestro consumer.

> Esta compilación de manifiesto solamente se puede hacer en el SDK de windows (en Visual Studio) abriendo una terminal.

```Shell
mc.exe -um myprovider.man
```

entonces una vez ejecutado el comando, nos genera el archivo. Este es importante para nuestro programa que se usa para enviar logs. Pero de eso hablaré más adelante.

También ocupamos generar una ```.dll``` esa se hace con los siguientes comandos:

```Shell
rc MyProvider.rc
```
y despues generamos la DLL

```
link /dll /noentry /machine:x64 MyProvider.res /out:MyProvider.dll
```

el ```/machine=x64``` funciona para generar ya sea una DLL de 32 o 64 bit.

Supongo que por nuestro archivo manifiesto de [```myprovider.man```](Provider/myprovider.man) busca la dll en esta ruta ```C:\Windows\System32``` debido por la siguientes lineas de código:

```
    <provider
    name="MyCustomETWProvider"
    guid="{6A282996-820B-42F3-8725-573FD309CD0A}"
    symbol="MY_ETW_PROVIDER"
    resourceFileName="%SystemRoot%\System32\MyProvider.dll"
    messageFileName="%SystemRoot%\System32\MyProvider.dll">
```

Ya en otra prueba las modifico.

Teniendo todo esto ya hecho registramos el ```Provider``` con la siguiente linea de comandos

> Esta ya la podemos realizar en una CMD con privilegios de Administrador

```Shell
wevtutil im MyProvider.man
```

Realizado esto podemos ya ver nuestro proveedor en el EventViewer o Visor de Eventos con el nombre MyCostumETWProvider. Lo bueno del Visor de Eventos es que puede enseñarte todos los logs que se enviaron y en la que se enviaron.

<center>
<img src=images/IMAGEN001.png width=50%> 
<img src=images/IMAGEN004.png width=75%>
</center>

Si lo queremos ver con otro programa puede ser con EtwExplorer de Pavel, el cual puede darnos más detalles cómo el XML Manifest del proveedor

<center>
<img src=images/IMAGEN002.png width=50%> 
<img src=images/IMAGEN003.png width=50%> 
</center>

Podría tomar ventaja de esto y ver cómo van los eventos de otros proveedores, por ejemplo del WindowsDefender para enviar LOGS, peroaparecen mis dudas ya que no sé si pueda generar desde un provider suplantado con un programa, eventos de algun tipo, pero vere que puedo hacer.

Nota si deseo eliminar el ```Provider``` registrado, se usa el siguiente comando en la CMD con privilegios de administrador. Donde se encuentra el archivo ```.man``` se utiliza este comando:

```Shell
wevtutil um MyProvider.man
```

sino tienes el archivo manifest, se elimina con el GUID

```Shell
wevtutil um {6a282996-820b-42f3-8725-573fd309cd0a}
```

## 3. Integrarlo en nuestro programa .EXE

Se usa el archivo de cabecera generado en la compilación de manifiesto, ayudandonos a simplificar ciertas partes de código.

En el código simple generamos nuestra GUID y lo registramos, pero con este lo único que debemos registrar es el ```REGHANDLE```, cómo el siguiente código:

```Cpp
static const GUID my_guid =
{ 0x6a282996,0x820b,0x42f3,{0x87,0x25,0x57,0x3f,0xd3,0x09,0xcd,0x0a} };

REGHANDLE g_EtwHandle = 0;

...

error = EventRegister(&my_guid, NULL, NULL, &g_EtwHandle);
```

Ahora solo se usa:

```Cpp
REGHANDLE g_EtwHandle = 0;

...

error = EventRegisterMyCustomETWProvider(&g_EtwHandle);
```

También la manera en como manda los descriptores ETW cambia con la librería, ya no tenemos que definirla cómo el código inicial:

```cpp
EVENT_DESCRIPTOR g_EventRestar =
{
    1,    // Id
    0,    // Version
    0,    // Channel
    4,    // Level (Information)
    0,    // Opcode
    0,    // Task
    0     // Keyword
};

...

EVENT_DATA_DESCRIPTOR data[2];

EventDataDescCreate(
    &data[0],
    &pid,
    sizeof(pid)
);

EventDataDescCreate(
    &data[1],
    text,
    (ULONG)((wcslen(text) + 1) * sizeof(WCHAR))
);

error = EventWrite(
    g_EtwHandle,
    &g_EventRestar,
    2,
    data
);
```

Ahora:

```cpp
DWORD pid = GetCurrentProcessId();
WCHAR text[] = L"Hola Mundo";

error = EventWriteProcessMessage(
    pid,
    text
);
```

Con la cabecera creada por el manifest (```.man```) nos ahorra cierto código ya generado y enviando los datos que se deben mandar por el ETW de manera correcta para poderse trasear.