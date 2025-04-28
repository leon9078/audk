/** @file

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Sbbr or SBBR   - Server Base Boot Requirements

  @par Reference(s):
    - Arm Server Base Boot Requirements 1.2, September 2019
**/

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/AcpiViewCommandLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"

#if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  #include "Arm/SbbrValidator.h"
#endif

STATIC UINT32  mTableCount;
STATIC UINT32  mBinTableCount;

/**
  This function dumps the ACPI table to a file.

  @param [in] Ptr         Pointer to the ACPI table data.
  @param [in] Length      The ACPI table Length.
  @param [in] OemTableId  The ACPI table Oem Table ID.

  @retval TRUE          Success.
  @retval FALSE         Failure.
**/
STATIC
BOOLEAN
DumpAcpiTableToFile (
  IN CONST UINT8   *Ptr,
  IN CONST UINTN   Length,
  IN CONST UINT64  OemTableId
  )
{
  CHAR16               FileNameBuffer[MAX_FILE_NAME_LEN];
  UINTN                TransferBytes;
  SELECTED_ACPI_TABLE  *SelectedTable;

  GetSelectedAcpiTable (&SelectedTable);

  UnicodeSPrint (
    FileNameBuffer,
    StrSize (FileNameBuffer),
    L".\\%c%c%c%c",
    CharToUpper (SelectedTable->Name[0]),
    CharToUpper (SelectedTable->Name[1]),
    CharToUpper (SelectedTable->Name[2]),
    CharToUpper (SelectedTable->Name[3])
    );

  if (SelectedTable->Type == EFI_ACPI_6_2_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
    CHAR16  SsdtFileNameBuffer[12];
    UINT8   *OemTableIdPtr;
    OemTableIdPtr = (UINT8 *)(UINTN)&OemTableId;

    UnicodeSPrint (
      SsdtFileNameBuffer,
      StrSize (SsdtFileNameBuffer),
      L"-%02u-%c%c%c%c%c%c%c%c",
      ++mBinTableCount,
      OemTableIdPtr[0],
      OemTableIdPtr[1],
      OemTableIdPtr[2],
      OemTableIdPtr[3],
      OemTableIdPtr[4],
      OemTableIdPtr[5],
      OemTableIdPtr[6],
      OemTableIdPtr[7]
      );

    while ((SsdtFileNameBuffer)[StrLen (SsdtFileNameBuffer) - 1] == L' ') {
      (SsdtFileNameBuffer)[StrLen (SsdtFileNameBuffer) - 1] = CHAR_NULL;
    }

    StrCatS (FileNameBuffer, MAX_FILE_NAME_LEN, SsdtFileNameBuffer);
  }

  StrCatS (FileNameBuffer, MAX_FILE_NAME_LEN, L".aml");

  Print (L"Dumping ACPI table to: %s (%u bytes) ... ", FileNameBuffer, Length);

  TransferBytes = ShellDumpBufferToFile (FileNameBuffer, Ptr, Length);
  return (Length == TransferBytes);
}

/**
  This function processes the table reporting options for the ACPI table.

  @param [in] Signature   The ACPI table Signature.
  @param [in] TablePtr    Pointer to the ACPI table data.
  @param [in] Length      The ACPI table Length.
  @param [in] OemTableId  The ACPI table Oem Table ID.

  @retval Returns TRUE if the ACPI table should be traced.
**/
BOOLEAN
ProcessTableReportOptions (
  IN CONST UINT32  Signature,
  IN CONST UINT8   *TablePtr,
  IN CONST UINT32  Length,
  IN CONST UINT64  OemTableId
  )
{
  UINTN                OriginalAttribute;
  UINT8                *SignaturePtr;
  BOOLEAN              Log;
  BOOLEAN              HighLight;
  SELECTED_ACPI_TABLE  *SelectedTable;

  //
  // set local variables to suppress incorrect compiler/analyzer warnings
  //
  OriginalAttribute = 0;
  SignaturePtr      = (UINT8 *)(UINTN)&Signature;
  Log               = FALSE;
  HighLight         = GetColourHighlighting ();
  GetSelectedAcpiTable (&SelectedTable);

  switch (GetReportOption ()) {
    case ReportAll:
      Log = TRUE;
      break;
    case ReportSelected:
      if (Signature == SelectedTable->Type) {
        Log                  = TRUE;
        SelectedTable->Found = TRUE;
      }

      break;
    case ReportTableList:
      if (mTableCount == 0) {
        if (HighLight) {
          OriginalAttribute = gST->ConOut->Mode->Attribute;
          gST->ConOut->SetAttribute (
                         gST->ConOut,
                         EFI_TEXT_ATTR (
                           EFI_CYAN,
                           ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)
                           )
                         );
        }

        Print (L"Installed Table(s):\n");
        if (HighLight) {
          gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
        }
      }

      Print (
        L"\t%4u. %c%c%c%c\n",
        ++mTableCount,
        SignaturePtr[0],
        SignaturePtr[1],
        SignaturePtr[2],
        SignaturePtr[3]
        );
      break;
    case ReportDumpBinFile:
      if (Signature == SelectedTable->Type) {
        SelectedTable->Found = TRUE;
        DumpAcpiTableToFile (TablePtr, Length, OemTableId);
      }

      break;
    case ReportMax:
      // We should never be here.
      // This case is only present to prevent compiler warning.
      break;
  } // switch

  if (Log) {
    if (HighLight) {
      OriginalAttribute = gST->ConOut->Mode->Attribute;
      gST->ConOut->SetAttribute (
                     gST->ConOut,
                     EFI_TEXT_ATTR (
                       EFI_LIGHTBLUE,
                       ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)
                       )
                     );
    }

    Print (
      L"\n\n --------------- %c%c%c%c Table --------------- \n\n",
      SignaturePtr[0],
      SignaturePtr[1],
      SignaturePtr[2],
      SignaturePtr[3]
      );
    if (HighLight) {
      gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
    }
  }

  return Log;
}

/**
  This function iterates the configuration table entries in the
  system table, retrieves the RSDP pointer and starts parsing the ACPI tables.

  @param [in] SystemTable Pointer to the EFI system table.

  @retval Returns EFI_NOT_FOUND   if the RSDP pointer is not found.
          Returns EFI_UNSUPPORTED if the RSDP version is less than 2.
          Returns EFI_SUCCESS     if successful.
**/
EFI_STATUS
EFIAPI
AcpiView (
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  UINTN                    Index;
  EFI_CONFIGURATION_TABLE  *EfiConfigurationTable;
  BOOLEAN                  FoundAcpiTable;
  UINTN                    OriginalAttribute;
  UINTN                    PrintAttribute;
  EREPORT_OPTION           ReportOption;
  UINT8                    *RsdpPtr;
  UINT32                   RsdpLength;
  UINT8                    RsdpRevision;
  PARSE_ACPI_TABLE_PROC    RsdpParserProc;
  BOOLEAN                  Trace;
  SELECTED_ACPI_TABLE      *SelectedTable;

  //
  // set local variables to suppress incorrect compiler/analyzer warnings
  //
  EfiConfigurationTable = NULL;
  OriginalAttribute     = 0;

  // Reset Table counts
  mTableCount    = 0;
  mBinTableCount = 0;

  // Reset The error/warning counters
  ResetErrorCount ();
  ResetWarningCount ();

  // Retrieve the user selection of ACPI table to process
  GetSelectedAcpiTable (&SelectedTable);

  // Search the table for an entry that matches the ACPI Table Guid
  FoundAcpiTable = FALSE;
  for (Index = 0; Index < SystemTable->NumberOfTableEntries; Index++) {
    if (CompareGuid (
          &gEfiAcpiTableGuid,
          &(SystemTable->ConfigurationTable[Index].VendorGuid)
          ))
    {
      EfiConfigurationTable = &SystemTable->ConfigurationTable[Index];
      FoundAcpiTable        = TRUE;
      break;
    }
  }

  if (FoundAcpiTable) {
    RsdpPtr = (UINT8 *)EfiConfigurationTable->VendorTable;

    // The RSDP revision is 1 byte starting at offset 15
    RsdpRevision = *(RsdpPtr + RSDP_REVISION_OFFSET);

    if (RsdpRevision < 2) {
      Print (
        L"ERROR: RSDP version less than 2 is not supported.\n"
        );
      return EFI_UNSUPPORTED;
    }

 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
    if (GetMandatoryTableValidate ()) {
      ArmSbbrResetTableCounts ();
    }

 #endif

    // The RSDP length is 4 bytes starting at offset 20
    RsdpLength = *(UINT32 *)(RsdpPtr + RSDP_LENGTH_OFFSET);

    Trace = ProcessTableReportOptions (RSDP_TABLE_INFO, RsdpPtr, RsdpLength, 0);

    Status = GetParser (RSDP_TABLE_INFO, &RsdpParserProc);
    if (EFI_ERROR (Status)) {
      Print (
        L"ERROR: No registered parser found for RSDP.\n"
        );
      return Status;
    }

    RsdpParserProc (
      Trace,
      RsdpPtr,
      RsdpLength,
      RsdpRevision
      );
  } else {
    IncrementErrorCount ();
    Print (
      L"ERROR: Failed to find ACPI Table Guid in System Configuration Table.\n"
      );
    return EFI_NOT_FOUND;
  }

 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (GetMandatoryTableValidate ()) {
    ArmSbbrReqsValidate ((ARM_SBBR_VERSION)GetMandatoryTableSpec ());
  }

 #endif

  ReportOption = GetReportOption ();
  if (ReportTableList != ReportOption) {
    if (((ReportSelected == ReportOption)  ||
         (ReportDumpBinFile == ReportOption)) &&
        (!SelectedTable->Found))
    {
      Print (L"Requested ACPI Table not found.\n");
    } else if (GetConsistencyChecking () &&
               (ReportDumpBinFile != ReportOption))
    {
      OriginalAttribute = gST->ConOut->Mode->Attribute;

      Print (L"Table Statistics:\n");

      if (GetColourHighlighting ()) {
        PrintAttribute = (GetErrorCount () > 0) ?
                         EFI_TEXT_ATTR (
                           EFI_RED,
                           ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)
                           ) :
                         OriginalAttribute;
        gST->ConOut->SetAttribute (gST->ConOut, PrintAttribute);
      }

      Print (L"\t%u Error(s)\n", GetErrorCount ());

      if (GetColourHighlighting ()) {
        PrintAttribute = (GetWarningCount () > 0) ?
                         EFI_TEXT_ATTR (
                           EFI_RED,
                           ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)
                           ) :
                         OriginalAttribute;

        gST->ConOut->SetAttribute (gST->ConOut, PrintAttribute);
      }

      Print (L"\t%u Warning(s)\n", GetWarningCount ());

      if (GetColourHighlighting ()) {
        gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
      }
    }
  }

  return EFI_SUCCESS;
}
