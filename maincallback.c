#include <efi.h>
#include <efilib.h>

EFI_GUID tcpGuid = EFI_TCP4_PROTOCOL;
EFI_SYSTEM_TABLE *gSystemTable;

void EFIAPI sent(EFI_EVENT event, void* context)
{
    Print(L"Data sent\n");
}

void EFIAPI accepted(EFI_EVENT event, void* context)
{
    Print(L">Context: %08x\n", context);

    EFI_TCP4_LISTEN_TOKEN *listenToken = (EFI_TCP4_LISTEN_TOKEN *)context;
    EFI_STATUS Status;

    Print(L"Event: %08x\n", event);
    Print(L"Connection accepted, new child handle: %08x\n", listenToken->NewChildHandle);
    Print(L"Completion token status: %d\n", listenToken->CompletionToken.Status);
    Print(L"Completion token event: %08x\n", listenToken->CompletionToken.Event);

    EFI_TCP4_IO_TOKEN transmitToken;
    EFI_TCP4_TRANSMIT_DATA txData;
    txData.Push = FALSE;
    txData.Urgent = FALSE;
    txData.DataLength = 10;
    txData.FragmentCount = 1;
    txData.FragmentTable[0].FragmentLength = 10;
    txData.FragmentTable[0].FragmentBuffer = L"Hello";

    Status = uefi_call_wrapper(gSystemTable->BootServices->CreateEvent, 5, EVT_NOTIFY_SIGNAL, EFI_TPL_CALLBACK, &sent, listenToken, &transmitToken.CompletionToken.Event);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not create event %d\n");
        return;
    }

    EFI_TCP4 *tcp4 = NULL;
    Status = uefi_call_wrapper(gSystemTable->BootServices->HandleProtocol, 3, listenToken->NewChildHandle, &tcpGuid, &tcp4);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not get TCP interface on new child %d\n", Status);
        return;
    }

    Status = uefi_call_wrapper(tcp4->Transmit, 2, listenToken->NewChildHandle, &transmitToken);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not send data %d\n");
        return;
    }
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    gSystemTable = SystemTable;

    Print(L"Hello, world!\n");

    EFI_STATUS Status;

    EFI_GUID tcpBindGuid = EFI_TCP4_SERVICE_BINDING_PROTOCOL;
    //EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    //EFI_GUID simplFS = SIMPLE_FILE_SYSTEM_PROTOCOL;

    EFI_TCP4_CONFIG_DATA configData;
    configData.TypeOfService = 1;
    configData.TimeToLive = 9;
    configData.AccessPoint.UseDefaultAddress = TRUE;
    configData.AccessPoint.ActiveFlag = FALSE;
    configData.AccessPoint.StationPort = 80;
    configData.ControlOption = NULL;
    
    Print(L"Searching for handles that support TCP binding\n");
    UINTN numberHandles = 10;
    EFI_HANDLE* handleBuffer = NULL;
    Status = uefi_call_wrapper(SystemTable->BootServices->LocateHandleBuffer, 5, ByProtocol, &tcpBindGuid, NULL, &numberHandles, &handleBuffer);
    if (EFI_ERROR(Status))
    {
        Print(L"Nothing supports TCP4 error %08x\n", Status);
        return EFI_ABORTED;
    }

    Print(L"%d handles support TCP4 service binding\n", numberHandles);

    Print(L"Attempting to get handle on protocol\n");
    EFI_SERVICE_BINDING* binder;
    Status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, handleBuffer[0], &tcpBindGuid, &binder);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not get handle (%d)\n", Status);
    }

    Print(L"Creating child to use\n");
    EFI_HANDLE child = NULL;
    Status = uefi_call_wrapper(binder->CreateChild, 2, binder, &child);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not create child (%d)\n", Status);
    }

    // Print(L"Attempting LocateProtocol\n");
    // EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    // EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    // Status = uefi_call_wrapper(SystemTable->BootServices->LocateProtocol,
    //                                3,
    //                                &gop_guid,
    //                                NULL,
    //                                &gop);
    // if (EFI_ERROR(Status))
    // {
    //     Print(L"LocateProtocol failed (%d)\n", Status);
    // }
    // else
    // {
    //     Print(L"Framebuffer base is at %lx\n", gop->Mode->FrameBufferBase);
    // }

    EFI_TCP4 *tcp4 = NULL;
    Status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, child, &tcpGuid, &tcp4);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not get handle to TCP4 protocol (%d)\n", Status);
    }

    // Print(L"LocateProtocol TCP4 Binding Service\n");
    // EFI_SERVICE_BINDING binder;
    // Status = uefi_call_wrapper(SystemTable->BootServices->LocateProtocol, 3, &tcpBindGuid, NULL, &binder);
    // if (EFI_ERROR(Status))
    // {
    //     Print(L"Could not locate TCP4 protocol (%d)\n", Status);
    // }

    Print(L"Attempting to configure TCP4\n");
    Status = uefi_call_wrapper(tcp4->Configure, 2, tcp4, &configData);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not configure TCP endpoint (%d)\n", Status);
        // TODO: probably want to wait for DHCP to complete
        return EFI_ABORTED;
    }

    void* buffer;
    Status = uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiBootServicesData, sizeof(EFI_TCP4_LISTEN_TOKEN), &buffer);
    if (EFI_ERROR(Status))
    {
        Print(L"Allocate Pool failed %d\n", Status);
        return EFI_ABORTED;
    }
    Print(L"Context: %08x\n", buffer);

    EFI_TCP4_LISTEN_TOKEN *listenToken = (EFI_TCP4_LISTEN_TOKEN *)buffer;
    Print(L"Listen token: %10x\n", listenToken);
    Print(L"Completion token: %10x\n", &(listenToken->CompletionToken));

    Print(L"Creating event for listen notification\n");
    Status = uefi_call_wrapper(SystemTable->BootServices->CreateEvent, 5, EVT_NOTIFY_WAIT, EFI_TPL_CALLBACK, &accepted, buffer, &listenToken->CompletionToken.Event);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not create event %d\n", Status);
        return EFI_ABORTED;
    }

    Print(L"Event: %08x\n", &(listenToken->CompletionToken.Event));

    Print(L"Accepting connection...\n");
    Status = uefi_call_wrapper(tcp4->Accept, 2, tcp4, listenToken);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not accept connection (%d)\n", Status);
    }

    // by creating a loop we yielf where WaitForEvent will block
    // Status = EFI_NOT_READY;
    // while (Status == EFI_NOT_READY)
    // {
    //     Status = uefi_call_wrapper(SystemTable->BootServices->CheckEvent, 1, SystemTable->ConIn->WaitForKey);
    //     if (Status != EFI_NOT_READY)
    //     {
    //         Print(L"%d\n", Status);
    //     }
    // }

    UINTN Index;
    EFI_EVENT events[2] = { SystemTable->ConIn->WaitForKey, listenToken->CompletionToken.Event };
    uefi_call_wrapper(SystemTable->BootServices->WaitForEvent, 3, 2, events, &Index);

    // if (Index == 1)
    // {
    //     uefi_call_wrapper(SystemTable->BootServices->WaitForEvent, 3, 1, &(SystemTable->ConIn->WaitForKey), &Index);
    // }

    SystemTable->BootServices->FreePool(handleBuffer);

    return EFI_SUCCESS;
}

