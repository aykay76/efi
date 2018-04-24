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

