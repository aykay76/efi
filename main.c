#include <efi.h>
#include <efilib.h>

EFI_GUID tcpGuid = EFI_TCP4_PROTOCOL;
EFI_SYSTEM_TABLE *gSystemTable;

void PrintState(EFI_TCP4 *tcp4)
{
    EFI_TCP4_CONNECTION_STATE connectionState;
    EFI_TCP4_CONFIG_DATA configData;
//    EFI_IPv4_MODE_DATA ipModeData;
    EFI_MANAGED_NETWORK_CONFIG_DATA mnConfigData;
    EFI_SIMPLE_NETWORK_MODE snMode;
    EFI_STATUS Status;

    Status = uefi_call_wrapper(tcp4->GetModeData, 6, tcp4, &connectionState, &configData, NULL, &mnConfigData, &snMode);
    if (Status == EFI_SUCCESS)
    {
        Print(L"Connection state: %d\n", connectionState);
        Print(L"Config data -> Access Point -> Active Flag: %d\n", configData.AccessPoint.ActiveFlag);
        Print(L"Address: %d.%d.%d.%d\n", configData.AccessPoint.StationAddress.Addr[0], configData.AccessPoint.StationAddress.Addr[1], configData.AccessPoint.StationAddress.Addr[2], configData.AccessPoint.StationAddress.Addr[3]);
    }
    else
    {
        Print(L"Error getting data %d\n", Status);
    }
}

BOOLEAN GotAddress(EFI_TCP4 *tcp4)
{
    EFI_TCP4_CONNECTION_STATE connectionState;
    EFI_TCP4_CONFIG_DATA configData;
//    EFI_IPv4_MODE_DATA ipModeData;
    EFI_MANAGED_NETWORK_CONFIG_DATA mnConfigData;
    EFI_SIMPLE_NETWORK_MODE snMode;
    EFI_STATUS Status;

    Status = uefi_call_wrapper(tcp4->GetModeData, 6, tcp4, &connectionState, &configData, NULL, &mnConfigData, &snMode);
    if (Status == EFI_SUCCESS)
    {
        if (configData.AccessPoint.StationAddress.Addr[0] == 0 &&
            configData.AccessPoint.StationAddress.Addr[1] == 0 &&
            configData.AccessPoint.StationAddress.Addr[2] == 0 &&
            configData.AccessPoint.StationAddress.Addr[3])
        {
            return FALSE;
        }
    }
    else
    {
        Print(L"Error getting data %d\n", Status);
        return FALSE;
    }

    return TRUE;
}

// void EFIAPI sent(EFI_EVENT event, void* context)
// {
//     Print(L"Data sent\n");
// }

// void EFIAPI accepted(EFI_EVENT event, void* context)
// {
//     Print(L">Context: %d\n", context);

//     EFI_TCP4_LISTEN_TOKEN *listenToken = (EFI_TCP4_LISTEN_TOKEN *)context;
//     EFI_STATUS Status;

//     Print(L"Event: %08x\n", event);
//     Print(L"Connection accepted, new child handle: %d\n", listenToken->NewChildHandle);
//     Print(L"Completion token status: %d\n", listenToken->CompletionToken.Status);
//     Print(L"Completion token event: %d\n", listenToken->CompletionToken.Event);

//     // EFI_TCP4_IO_TOKEN transmitToken;
//     // EFI_TCP4_TRANSMIT_DATA txData;
//     // txData.Push = FALSE;
//     // txData.Urgent = FALSE;
//     // txData.DataLength = 10;
//     // txData.FragmentCount = 1;
//     // txData.FragmentTable[0].FragmentLength = 10;
//     // txData.FragmentTable[0].FragmentBuffer = L"Hello";

//     Status = uefi_call_wrapper(gSystemTable->BootServices->CreateEvent, 5, EVT_NOTIFY_WAIT, EFI_TPL_CALLBACK, &sent, listenToken, &transmitToken.CompletionToken.Event);
//     if (EFI_ERROR(Status))
//     {
//         Print(L"Could not create event %d\n");
//         return;
//     }

//     EFI_TCP4 *tcp4 = NULL;
//     Status = uefi_call_wrapper(gSystemTable->BootServices->HandleProtocol, 3, listenToken->NewChildHandle, &tcpGuid, &tcp4);
//     if (EFI_ERROR(Status))
//     {
//         Print(L"Could not get TCP interface on new child %d\n", Status);
//         return;
//     }

//     Status = uefi_call_wrapper(tcp4->Transmit, 2, listenToken->NewChildHandle, &transmitToken);
//     if (EFI_ERROR(Status))
//     {
//         Print(L"Could not send data %d\n");
//         return;
//     }
// }

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

    EFI_TCP4 *tcp4 = NULL;
    Status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, child, &tcpGuid, &tcp4);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not get handle to TCP4 protocol (%d)\n", Status);
    }

    // Print(L"Waiting for IP address.");
    // while (!GotAddress(tcp4))
    // {
    //     // wait for a bit
    //     UINTN index = 0;
    //     EFI_EVENT myEvent;
    //     Status = uefi_call_wrapper(SystemTable->BootServices->CreateEvent, 5, EVT_TIMER, TPL_CALLBACK, (EFI_EVENT_NOTIFY)NULL, (VOID*)NULL, &myEvent);
    //     Status = uefi_call_wrapper(SystemTable->BootServices->SetTimer, 3, myEvent, TimerPeriodic, 1e7);
    //     Status = uefi_call_wrapper(SystemTable->BootServices->WaitForEvent, 3, 1, &myEvent, &index);
    //     Print(L".");
    // }
    // Print(L"\nGot an IP address, ready to listen!\n");

    Status = uefi_call_wrapper(tcp4->Configure, 2, tcp4, &configData);
    while (EFI_ERROR(Status))
    {
        UINTN index = 0;
        EFI_EVENT myEvent;
        Status = uefi_call_wrapper(SystemTable->BootServices->CreateEvent, 5, EVT_TIMER, TPL_CALLBACK, (EFI_EVENT_NOTIFY)NULL, (VOID*)NULL, &myEvent);
        Status = uefi_call_wrapper(SystemTable->BootServices->SetTimer, 3, myEvent, TimerPeriodic, 1e7);
        Status = uefi_call_wrapper(SystemTable->BootServices->WaitForEvent, 3, 1, &myEvent, &index);
        Print(L".");
        Status = uefi_call_wrapper(tcp4->Configure, 2, tcp4, &configData);
    }

    Print(L"Ready to listen.\n");
    PrintState(tcp4);

    void* buffer;
    Status = uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiBootServicesData, sizeof(EFI_TCP4_LISTEN_TOKEN), &buffer);
    if (EFI_ERROR(Status))
    {
        Print(L"Allocate Pool failed %d\n", Status);
        return EFI_ABORTED;
    }
    Print(L"Context: %d\n", buffer);

    EFI_TCP4_LISTEN_TOKEN *listenToken = (EFI_TCP4_LISTEN_TOKEN *)buffer;

    Print(L"Creating event for listen notification\n");
    Status = uefi_call_wrapper(SystemTable->BootServices->CreateEvent, 5, 0, EFI_TPL_APPLICATION, NULL, NULL, &listenToken->CompletionToken.Event);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not create event %d\n", Status);
        return EFI_ABORTED;
    }

    Print(L"Accepting connection...\n");
    Status = uefi_call_wrapper(tcp4->Accept, 2, tcp4, listenToken);
    if (EFI_ERROR(Status))
    {
        Print(L"Could not accept connection (%d)\n", Status);
    }

    for (;;)
    {
        Status = uefi_call_wrapper(SystemTable->BootServices->CheckEvent, 1, listenToken->CompletionToken.Event);

        if (Status == EFI_SUCCESS)
        {
            PrintState(tcp4);

            EFI_TCP4 *childTcp = NULL;
            Status = uefi_call_wrapper(gSystemTable->BootServices->HandleProtocol, 3, listenToken->NewChildHandle, &tcpGuid, &childTcp);
            if (EFI_ERROR(Status))
            {
                Print(L"Could not get TCP interface on new child %d\n", Status);
                return EFI_ABORTED;
            }

            EFI_TCP4_IO_TOKEN transmitToken;
            EFI_TCP4_TRANSMIT_DATA txData;
            txData.Push = FALSE;
            txData.Urgent = FALSE;
            txData.DataLength = 10;
            txData.FragmentCount = 1;
            txData.FragmentTable[0].FragmentLength = 10;
            txData.FragmentTable[0].FragmentBuffer = L"Hello";
            transmitToken.Packet.TxData = &txData;

            Status = uefi_call_wrapper(gSystemTable->BootServices->CreateEvent, 5, 0, EFI_TPL_APPLICATION, NULL, NULL, &transmitToken.CompletionToken.Event);
            if (EFI_ERROR(Status))
            {
                Print(L"Could not create event %d\n");
                return EFI_ABORTED;
            }

            Status = uefi_call_wrapper(childTcp->Transmit, 2, childTcp, &transmitToken);
            if (EFI_ERROR(Status))
            {
                Print(L"Could not send data %d\n", Status);
                return EFI_ABORTED;
            }

            /////////////////////////////////////////
            // get ready to accept another connection
            // Status = uefi_call_wrapper(SystemTable->BootServices->CloseEvent, 1, listenToken->CompletionToken.Event);
            // if (EFI_ERROR(Status))
            // {
            //     Print(L"Could not close event %d\n", Status);
            // }

            // Status = uefi_call_wrapper(SystemTable->BootServices->FreePool, 1, listenToken);
            // if (EFI_ERROR(Status))
            // {
            //     Print(L"Could not free memory %d\n", Status);
            // }

            // void* buffer;
            // Status = uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiBootServicesData, sizeof(EFI_TCP4_LISTEN_TOKEN), &buffer);
            // if (EFI_ERROR(Status))
            // {
            //     Print(L"Allocate Pool failed %d\n", Status);
            //     return EFI_ABORTED;
            // }
            // Print(L"Context: %d\n", buffer);

            // EFI_TCP4_LISTEN_TOKEN *listenToken = (EFI_TCP4_LISTEN_TOKEN *)buffer;

            // Status = uefi_call_wrapper(SystemTable->BootServices->CreateEvent, 5, 0, EFI_TPL_APPLICATION, NULL, NULL, &listenToken->CompletionToken.Event);
            // if (EFI_ERROR(Status))
            // {
            //     Print(L"Could not create listen event %d\n", Status);
            //     return EFI_ABORTED;
            // }

            Status = uefi_call_wrapper(tcp4->Accept, 2, tcp4, listenToken);
            if (EFI_ERROR(Status))
            {
                Print(L"Could not accept connection (%d)\n", Status);
                return EFI_ABORTED;
            }
        }
        if (Status == EFI_NOT_READY)
        {
            // wait for a bit
            UINTN index = 0;
            EFI_EVENT myEvent;
            Status = uefi_call_wrapper(SystemTable->BootServices->CreateEvent, 5, EVT_TIMER, TPL_CALLBACK, (EFI_EVENT_NOTIFY)NULL, (VOID*)NULL, &myEvent);
            Status = uefi_call_wrapper(SystemTable->BootServices->SetTimer, 3, myEvent, TimerPeriodic, 1000000);
            Status = uefi_call_wrapper(SystemTable->BootServices->WaitForEvent, 3, 1, &myEvent, &index);
        }
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

