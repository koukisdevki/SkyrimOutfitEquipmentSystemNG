Scriptname SkyOutEquSysMCM extends SKI_ConfigBase Hidden

Int      _iOutfitBrowserPage   = 0
Int      _iOutfitNameMaxBytes = 256 ; init option, should never change at run-time; can change if the DLL is revised appropriately
Int _OutfitsPageMaxOutfits ; init option
String[] _sOutfitNames
String   _sSelectedOutfit = ""

Actor    _aCurrentActor
Actor[]  _kActorSelection_SelectCandidates

String   _sEditingOutfit = ""
String   _sOutfitShowingContextMenu = ""
Int      _iOutfitEditorBodySlotPage = 0

String[] _sOutfitSlotNames
String[] _sOutfitSlotArmors
Armor[]  _kOutfitSlotArmors
Armor[]  _kOutfitContents

Bool     _sOutfitShowingSlotEditor = False

String[] _sOutfitEditor_AddCandidates
Armor[]  _kOutfitEditor_AddCandidates

String[] _sOutfitEditor_AddFromListCandidates
Armor[]  _kOutfitEditor_AddFromListCandidates
String   _sOutfitEditor_AddFromList_Filter   = ""
Bool     _bOutfitEditor_AddFromList_Playable = True

Int _iSelectMenuMax ; init option

; outfit page
Int _OutfitNamesPage ; init option
Int _iOutfitNames_HeaderOptionCount ; init option
Int _iOutfitNames_PageStartIndex ; init option

; optionally reset on config open
Int _OutfitImportModPage ; init option
Int _iOutfitImporter_PageStartIndex ; init option
String[] _sOutfitImporter_ModList
String _sOutfitImporter_SelectedMod = "" ; init option
Int _iOutfitImporter_HeaderOptionCount ; init option

Int _OutfitImportOutfitsForModPage ; init option
Int _iOutfitForModImporter_PageStartIndex ; init option
String[] _sOutfitImporter_AddOutfitsForModCandidates
Int _iOutfitForModImporter_HeaderOptionCount ; init option

Int _PlayerInventoryManagementMode; init option
Int _NPCInventoryManagementMode; init option

Function AddLocationOptions(Int[] aiIndices, String sHeaderKey)
   AddHeaderOption(sHeaderKey)
   
   Int iCount = aiIndices.Length
   Int iIterator = 0
   
   While iIterator < iCount
       String sLocationOutfit = SkyrimOutfitEquipmentSystemNativeFuncs.GetLocationOutfit(_aCurrentActor, aiIndices[iIterator])
       If sLocationOutfit == ""
           sLocationOutfit = "$SkyOutEquSys_AutoswitchEdit_None"
       EndIf
       AddMenuOptionST("OPT_AutoswitchEntry" + aiIndices[iIterator], "$SkyOutEquSys_Text_Autoswitch" + aiIndices[iIterator], sLocationOutfit)
       iIterator = iIterator + 1
   EndWhile
EndFunction

Int Function GetModVersion() Global ; static method; therefore, safely callable by outside parties even before/during OnInit
	Return 0x01000000
EndFunction
Int Function GetVersion()
	Return GetModVersion()
EndFunction

SkyOutEquSysQuickslotManager Function GetQuickslotManager() Global
   Return Quest.GetQuest("SkyrimOutfitEquipmentSystemQuickslotManager") as SkyOutEquSysQuickslotManager
EndFunction

String Function InventoryModeToTrString(int inventoryMode)
   If inventoryMode == 1
      return "$SkyOutEquSys_InventoryManagementMode_Automatic"
   ElseIf inventoryMode == 2
      return "$SkyOutEquSys_InventoryManagementMode_Immersive"
   EndIf
	Return ""
EndFunction

Event OnGameReload()
	Parent.OnGameReload()
EndEvent
Event OnConfigInit()
EndEvent
Event OnConfigOpen()
   InitializeOptions()
   _iOutfitNameMaxBytes = SkyrimOutfitEquipmentSystemNativeFuncs.GetOutfitNameMaxLength()
   _sOutfitImporter_ModList = SkyrimOutfitEquipmentSystemNativeFuncs.GetAllLoadedModsList()
   ResetOutfitBrowser()
   ResetOutfitEditor()
   RefreshCache()
EndEvent
Event OnConfigClose()
   If SkyrimOutfitEquipmentSystemNativeFuncs.IsEnabled()
      SkyrimOutfitEquipmentSystemNativeFuncs.SetOutfitsUsingLocation(Game.GetPlayer().GetCurrentLocation(), Weather.GetCurrentWeather())
      SkyrimOutfitEquipmentSystemNativeFuncs.RefreshArmorForAllConfiguredActors()
      ResetOutfitBrowser()
      ResetOutfitEditor()
      SkyrimOutfitEquipmentSystemNativeFuncs.AutoOutfitSwitchStateReset()
   Endif
EndEvent
Event OnPageReset(String asPage)
   If asPage == "$SkyOutEquSys_MCM_Options"
      ResetOutfitEditor()
      ShowOptions()
   ElseIf asPage == "$SkyOutEquSys_MCM_OutfitList"
      If _sEditingOutfit
         ShowOutfitEditor()
      Else
         ResetOutfitEditor()
         ShowOutfitList()
      EndIf
   EndIf
EndEvent

Function InitializeOptions()
   _OutfitsPageMaxOutfits = 20 ; init option
   _iSelectMenuMax = 1000 ; init option
   ; outfit page
   _OutfitNamesPage = 1 ; init option
   _iOutfitNames_HeaderOptionCount = 0 ; init option
   _iOutfitNames_PageStartIndex = 0 ; init option
   ; optionally reset on config open
   _OutfitImportModPage   = 1 ; init option
   _iOutfitImporter_PageStartIndex = 0 ; init option
   _sOutfitImporter_SelectedMod = "" ; init option
   _iOutfitImporter_HeaderOptionCount = 0 ; init option
   _OutfitImportOutfitsForModPage = 1 ; init option
   _iOutfitForModImporter_PageStartIndex = 0 ; init option
   _iOutfitForModImporter_HeaderOptionCount = 0 ; init option

   ; init options
   _PlayerInventoryManagementMode = SkyrimOutfitEquipmentSystemNativeFuncs.GetPlayerInventoryManagementMode(); init option
   _NPCInventoryManagementMode = SkyrimOutfitEquipmentSystemNativeFuncs.GetNPCInventoryManagementMode(); init option
EndFunction

Function FullRefresh()
   ResetOutfitBrowser()
   ResetOutfitEditor()
   RefreshCache()
   ForcePageReset()
EndFunction

Function RefreshCache()
   _sOutfitNames    = SkyrimOutfitEquipmentSystemNativeFuncs.ListOutfits()
   _sSelectedOutfit = SkyrimOutfitEquipmentSystemNativeFuncs.GetSelectedOutfit(_aCurrentActor)
   ;
   _sOutfitNames = SkyrimOutfitEquipmentSystemNativeFuncs.NaturalSort_ASCII(_sOutfitNames)


EndFunction

Function ResetOutfitBrowser()
   _iOutfitBrowserPage   = 0
   _iOutfitEditorBodySlotPage = 0
   _kActorSelection_SelectCandidates = new Actor[1]
   _aCurrentActor = Game.GetPlayer()
   _sEditingOutfit       = ""
   _sOutfitShowingContextMenu = ""
   _sOutfitNames = new String[1]
   _sOutfitShowingSlotEditor = False
EndFunction
Function ResetOutfitEditor()
   _sOutfitSlotNames  = new String[1]
   _sOutfitSlotArmors = new String[1]
   _kOutfitSlotArmors = new Armor[1]
   _kOutfitContents   = new Armor[1]
   _sOutfitEditor_AddCandidates = new String[1]
   _kOutfitEditor_AddCandidates = new Armor[1]
   ;
   _sOutfitEditor_AddFromListCandidates = new String[1]
   _kOutfitEditor_AddFromListCandidates = new Armor[1]
   _sOutfitEditor_AddFromList_Filter   = ""
   _bOutfitEditor_AddFromList_Playable = True
EndFunction

;/Block/; ; Helpers
   Int Function BodySlotToMask(Int aiSlot) Global
      Return Math.LeftShift(1, aiSlot - 30)
   EndFunction
   String Function BodySlotName(Int aiSlot) Global
      Return "$SkyOutEquSys_BodySlot" + aiSlot
   EndFunction
   Function LogStringArrayForDebugging(String asPrefix, String[] asArray) Global
      Debug.Trace(asPrefix)
      Int iIterator = 0
      While iIterator < asArray.Length
         Debug.Trace("[" + iIterator + "] == " + asArray[iIterator])
         iIterator = iIterator + 1
      EndWhile
   EndFunction
   String[] Function PrependStringToArray(String[] asMenu, String asPrepend)
      Int      iCount = asMenu.Length
      String[] kOut   = Utility.CreateStringArray(iCount + 1)
      kOut[0] = asPrepend
      Int iIterator = 0
      While iIterator < iCount
         Int iTemporary = iIterator + 1
         kOut[iTemporary] = asMenu[iIterator]
         iIterator = iTemporary
      EndWhile
      Return kOut
   EndFunction
;/EndBlock/;

Function SetupSlotDataForOutfit(String asOutfitName)
   _kOutfitContents = SkyrimOutfitEquipmentSystemNativeFuncs.GetOutfitContents(asOutfitName)
   ;
   ; This process is doable in Papyrus, but not without a significant 
   ; performance hit. It's fast in the DLL.
   ;
   SkyrimOutfitEquipmentSystemNativeFuncs.PrepOutfitBodySlotListing(asOutfitName)
   Int[] iSlots = SkyrimOutfitEquipmentSystemNativeFuncs.GetOutfitBodySlotListingSlotIndices()
   _sOutfitSlotNames = Utility.CreateStringArray(iSlots.Length)
   Int iIterator = 0
   While iIterator < iSlots.Length
      _sOutfitSlotNames[iIterator] = BodySlotName(iSlots[iIterator])
      iIterator = iIterator + 1
   EndWhile
   _sOutfitSlotArmors = SkyrimOutfitEquipmentSystemNativeFuncs.GetOutfitBodySlotListingArmorNames()
   _kOutfitSlotArmors = SkyrimOutfitEquipmentSystemNativeFuncs.GetOutfitBodySlotListingArmorForms()
   SkyrimOutfitEquipmentSystemNativeFuncs.ClearOutfitBodySlotListing()
EndFunction

;/Block/; ; Default handlers
   Event OnSelectST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 16) == "OutfitList_Item_"
         String sOutfitName = StringUtil.Substring(sState, 16)
         _sOutfitShowingContextMenu = sOutfitName
         ForcePageReset()
         Return
      EndIf
      If StringUtil.Substring(sState, 0, 22) == "OutfitEditor_BodySlot_"
         Int iEntryIndex = (_iOutfitEditorBodySlotPage*8) + (StringUtil.Substring(sState, 22) as Int)
         String sArmorName = _sOutfitSlotArmors[iEntryIndex]
         Armor  kArmorForm = _kOutfitSlotArmors[iEntryIndex]
         Bool bDelete = ShowMessage("$SkyOutEquSys_Confirm_RemoveArmor_Text{" + sArmorName + "}", True, "$SkyOutEquSys_Confirm_RemoveArmor_Yes", "$SkyOutEquSys_Confirm_RemoveArmor_No")
         If bDelete
            SkyrimOutfitEquipmentSystemNativeFuncs.RemoveArmorFromOutfit(_sEditingOutfit, kArmorForm)
            SetupSlotDataForOutfit(_sEditingOutfit)
            ForcePageReset()
         EndIf
         Return
      EndIf
   EndEvent
   Event OnMenuOpenST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 19) == "OPT_AutoswitchEntry"
          ; Calculate total outfits and pages
          Int totalOutfits = _sOutfitNames.Length
          Int totalPages = (totalOutfits + _iSelectMenuMax - 1) / _iSelectMenuMax
          
          ; Calculate start and end indices for the current page
          Int startIndex = (_OutfitNamesPage - 1) * _iSelectMenuMax
          
          ; Calculate end index with a simple comparison
          Int maxEndIndex = totalOutfits - 1
          Int calculatedEndIndex = startIndex + _iSelectMenuMax - 1
          Int endIndex = calculatedEndIndex
          
          ; Ensure we don't exceed the array bounds
          If endIndex > maxEndIndex
              endIndex = maxEndIndex
          EndIf
          
          ; Calculate how many items will be on this page
          Int itemsOnPage = endIndex - startIndex + 1
          
          ; Add navigation buttons conditionally
          Bool hasPrevPage = (_OutfitNamesPage > 1) && totalPages > 1
          Bool hasNextPage = (_OutfitNamesPage < totalPages) && totalPages > 1
          
          ; Count navigation options
          Int navOptionsCount = 0
          If hasPrevPage
              navOptionsCount += 1
          EndIf
          If hasNextPage
              navOptionsCount += 1
          EndIf
          
          ; Calculate total menu size: 
          ; cancel button + "None" option + nav options + outfit entries for the page
          Int menuSize = 1 + 1 + navOptionsCount + itemsOnPage
          
          ; Create menu array with exact size
          String[] sMenu = Utility.CreateStringArray(menuSize)
          
          ; Set cancel option
          sMenu[0] = "$SkyOutEquSys_AutoswitchEdit_Cancel"
          
          ; Set None option
          sMenu[1] = "$SkyOutEquSys_AutoswitchEdit_None"
          
          ; Current position in the menu array
          Int menuIndex = 2
          
          ; Add navigation buttons
          If hasPrevPage
              sMenu[menuIndex] = "$SkyOutEquSys_MCMText_PrevPageOption"
              menuIndex += 1
          EndIf
          
          If hasNextPage
              sMenu[menuIndex] = "$SkyOutEquSys_MCMText_NextPageOption"
              menuIndex += 1
          EndIf
          
          ; Store the number of header options (cancel + none + nav options)
          Int headerOptionsCount = 2 + navOptionsCount
          
          ; Add outfit entries for the current page
          Int i = 0
          While i < itemsOnPage
              sMenu[menuIndex] = _sOutfitNames[startIndex + i]
              menuIndex += 1
              i += 1
          EndWhile
          
          ; Set the menu options
          SetMenuDialogOptions(sMenu)
          SetMenuDialogStartIndex(0)
          SetMenuDialogDefaultIndex(0)
          
          ; Store values for the OnAccept handler
          _iOutfitNames_HeaderOptionCount = headerOptionsCount
          _iOutfitNames_PageStartIndex = startIndex
          Return
      EndIf
  EndEvent
  Event OnMenuAcceptST(Int aiIndex)
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 19) == "OPT_AutoswitchEntry"
         ; Get navigation state
         Int totalOutfits = _sOutfitNames.Length
         Int totalPages = (totalOutfits + _iSelectMenuMax - 1) / _iSelectMenuMax
         Bool hasPrevPage = (_OutfitNamesPage > 1) && totalPages > 1
         Bool hasNextPage = (_OutfitNamesPage < totalPages) && totalPages > 1
         
         If aiIndex == 0 ; user canceled
            Return
         EndIf
         
         If aiIndex == 1 ; user wants no outfit
            Int iAutoswitchIndex = StringUtil.Substring(sState, 19) as Int
            SkyrimOutfitEquipmentSystemNativeFuncs.UnsetLocationOutfit(_aCurrentActor, iAutoswitchIndex)
            SetMenuOptionValueST("$SkyOutEquSys_AutoswitchEdit_None")
            Return
         EndIf
         
         ; Check for navigation buttons
         Int navOffset = 2 ; Start with 2 to account for Cancel and None options
         
         ; Check if Previous Page was selected
         If hasPrevPage && aiIndex == navOffset
            _OutfitNamesPage -= 1
            SetMenuDialogOptions(new String[1]) ; Force menu to reopen
            ShowMessage("$SkyOutEquSys_MCMText_PrevPageOptionMessage", False, "$SkyOutEquSys_MessageDismiss")
            Return
         EndIf
         navOffset += hasPrevPage as Int ; Move offset if we have a prev button
         
         ; Check if Next Page was selected
         If hasNextPage && aiIndex == navOffset
            _OutfitNamesPage += 1
            SetMenuDialogOptions(new String[1]) ; Force menu to reopen
            ShowMessage("$SkyOutEquSys_MCMText_NextPageOptionMessage", False, "$SkyOutEquSys_MessageDismiss")
            Return
         EndIf
         
         ; If we get here, an outfit was selected
         Int outfitIndex = aiIndex - _iOutfitNames_HeaderOptionCount + _iOutfitNames_PageStartIndex
         If outfitIndex >= 0 && outfitIndex < _sOutfitNames.Length
            ; Set the requested outfit
            String sOutfitName = _sOutfitNames[outfitIndex]
            Int iAutoswitchIndex = StringUtil.Substring(sState, 19) as Int
            SkyrimOutfitEquipmentSystemNativeFuncs.SetLocationOutfit(_aCurrentActor, iAutoswitchIndex, sOutfitName)
            SetMenuOptionValueST(SkyrimOutfitEquipmentSystemNativeFuncs.GetLocationOutfit(_aCurrentActor, iAutoswitchIndex))
         EndIf
         Return
      EndIf
   EndEvent
   Event OnHighlightST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 19) == "OPT_AutoswitchEntry"
         SetInfoText("$SkyOutEquSys_Desc_Autoswitch")
         Return
      EndIf
      If StringUtil.Substring(sState, 0, 16) == "OutfitList_Item_"
         SetInfoText("$SkyOutEquSys_MCMInfoText_Outfit")
         Return
      EndIf
      If StringUtil.Substring(sState, 0, 22) == "OutfitEditor_BodySlot_"
         SetInfoText("$SkyOutEquSys_MCMInfoText_BodySlot")
         Return
      EndIf
   EndEvent
   Event OnDefaultST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 19) == "OPT_AutoswitchEntry"
         Int iAutoswitchIndex = StringUtil.Substring(sState, 19) as Int
         Bool bDelete = ShowMessage("$SkyOutEquSys_Confirm_UnsetAutoswitch_Text", True, "$SkyOutEquSys_Confirm_UnsetAutoswitch_Yes", "$SkyOutEquSys_Confirm_UnsetAutoswitch_No")
         If bDelete
            SkyrimOutfitEquipmentSystemNativeFuncs.UnsetLocationOutfit(_aCurrentActor, iAutoswitchIndex)
            SetMenuOptionValueST("")
         EndIf
         Return
      EndIf
   EndEvent
;/EndBlock/;

;/Block/; ; Options
   Function ShowOptions()
      ;/Block/; ; Left column
         SetCursorFillMode(TOP_TO_BOTTOM)
         SetCursorPosition(0)
         If SKSE.GetPluginVersion("SkyrimOutfitEquipmentSystemNG") == -1
            AddHeaderOption("$SkyOutEquSys_Text_WarningHeader")
            return
         EndIf
         AddToggleOptionST("OPT_Enabled", "$Enabled", SkyrimOutfitEquipmentSystemNativeFuncs.IsEnabled())
         AddMenuOptionST("OPT_SelectActorSelection", "$SkyOutEquSys_Text_SelectActorSelection", _aCurrentActor.GetBaseObject().GetName())
         AddEmptyOption()
         ;
         ; Active actor selection
         ;
         AddHeaderOption("$SkyOutEquSys_Text_ActiveActorHeader")
         AddMenuOptionST("OPT_AddActorSelection", "$SkyOutEquSys_Text_AddActorSelection", "")
         AddMenuOptionST("OPT_RemoveActorSelection", "$SkyOutEquSys_Text_RemoveActorSelection", "")
         AddEmptyOption()
         ;
         ; Inventory Management:
         ;
         AddHeaderOption("$SkyOutEquSys_MCMHeader_InventoryManagement")
         AddMenuOptionST("OPT_PlayerInventoryManagementMode", "$SkyOutEquSys_Text_PlayerInventoryManagementMode", InventoryModeToTrString(_PlayerInventoryManagementMode))
         AddMenuOptionST("OPT_NPCInventoryManagementMode", "$SkyOutEquSys_Text_NPCInventoryManagementMode", InventoryModeToTrString(_NPCInventoryManagementMode))
         AddEmptyOption()
         ;
         ; Quickslots:
         ;
         SkyOutEquSysQuickslotManager kQM = GetQuickslotManager()
         AddHeaderOption("$SkyOutEquSys_MCMHeader_Quickslots")
         AddToggleOptionST("OPT_QuickslotsEnabled", "$SkyOutEquSys_Text_EnableQuickslots", kQM.GetEnabled())
         AddEmptyOption()
         ;
         ; Setting import/export
         ;
         AddHeaderOption("$SkyOutEquSys_Text_SettingExportImport")
         AddTextOptionST ("OPT_ExportSettings", "$SkyOutEquSys_Text_Export", "")
         AddTextOptionST ("OPT_ImportSettings", "$SkyOutEquSys_Text_Import", "")
      ;/EndBlock/;
      ;/Block/; ; Right column
         SetCursorPosition(1)

         AddLocationOptions(SkyrimOutfitEquipmentSystemNativeFuncs.GetAutoSwitchActionBasedLocationArray(), "$SkyOutEquSys_MCMHeader_Autoswitch_Action")
         AddLocationOptions(SkyrimOutfitEquipmentSystemNativeFuncs.GetAutoSwitchGenericLocationArray(), "$SkyOutEquSys_MCMHeader_Autoswitch_Generic")
         AddLocationOptions(SkyrimOutfitEquipmentSystemNativeFuncs.GetAutoSwitchSpecificLocationArray(), "$SkyOutEquSys_MCMHeader_Autoswitch_Specific")
      ;/EndBlock/;

   EndFunction
   ;
   State OPT_Enabled
      Event OnSelectST()
         Bool bToggle = !SkyrimOutfitEquipmentSystemNativeFuncs.IsEnabled()
         SkyrimOutfitEquipmentSystemNativeFuncs.SetEnabled(bToggle)
         SetToggleOptionValueST(bToggle)
      EndEvent
   EndState
   State OPT_AddActorSelection
      Event OnMenuOpenST()
         _kActorSelection_SelectCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.ActorNearPC()
         String[] kActorNames = Utility.CreateStringArray(_kActorSelection_SelectCandidates.Length)
         Int iIterator = 0
         While iIterator < _kActorSelection_SelectCandidates.Length
            kActorNames[iIterator] = _kActorSelection_SelectCandidates[iIterator].GetActorBase().GetName()
            iIterator = iIterator + 1
         EndWhile
         String[] sMenu = PrependStringToArray(kActorNames, "$SkyOutEquSys_OEdit_AddCancel")
         SetMenuDialogOptions(sMenu)
         SetMenuDialogStartIndex(0)
         SetMenuDialogDefaultIndex(0)
      EndEvent
      Event OnMenuAcceptST(Int aiIndex)
         If aiIndex == 0 || aiIndex > _kActorSelection_SelectCandidates.Length
            return
         Endif
         SkyrimOutfitEquipmentSystemNativeFuncs.AddActor(_kActorSelection_SelectCandidates[aiIndex - 1])
      EndEvent
      Event OnDefaultST()
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutEquSys_Desc_AddActor")
      EndEvent
   EndState
   State OPT_RemoveActorSelection
      Event OnMenuOpenST()
         _kActorSelection_SelectCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.ListActors()
         String[] kActorNames = Utility.CreateStringArray(_kActorSelection_SelectCandidates.Length)
         Int iIterator = 0
         While iIterator < _kActorSelection_SelectCandidates.Length
            kActorNames[iIterator] = _kActorSelection_SelectCandidates[iIterator].GetActorBase().GetName()
            iIterator = iIterator + 1
         EndWhile
         String[] sMenu = PrependStringToArray(kActorNames, "$SkyOutEquSys_OEdit_AddCancel")
         SetMenuDialogOptions(sMenu)
         SetMenuDialogStartIndex(0)
         SetMenuDialogDefaultIndex(0)
      EndEvent
      Event OnMenuAcceptST(Int aiIndex)
         If aiIndex == 0 || aiIndex > _kActorSelection_SelectCandidates.Length
            return
         Endif
         SkyrimOutfitEquipmentSystemNativeFuncs.RemoveActor(_kActorSelection_SelectCandidates[aiIndex - 1])
         SkyrimOutfitEquipmentSystemNativeFuncs.RefreshArmorFor(_kActorSelection_SelectCandidates[aiIndex - 1])
      EndEvent
      Event OnDefaultST()
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutEquSys_Desc_RemoveActor")
      EndEvent
   EndState
   State OPT_SelectActorSelection
      Event OnMenuOpenST()
         _kActorSelection_SelectCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.ListActors()
         String[] kActorNames = Utility.CreateStringArray(_kActorSelection_SelectCandidates.Length)
         Int iIterator = 0
         While iIterator < _kActorSelection_SelectCandidates.Length
            kActorNames[iIterator] = _kActorSelection_SelectCandidates[iIterator].GetActorBase().GetName()
            iIterator = iIterator + 1
         EndWhile
         String[] sMenu = PrependStringToArray(kActorNames, "$SkyOutEquSys_OEdit_AddCancel")
         SetMenuDialogOptions(sMenu)
         SetMenuDialogStartIndex(0)
         SetMenuDialogDefaultIndex(0)
      EndEvent
      Event OnMenuAcceptST(Int aiIndex)
         If aiIndex == 0 || aiIndex > _kActorSelection_SelectCandidates.Length
            return
         Endif
         _aCurrentActor = _kActorSelection_SelectCandidates[aiIndex - 1]
         RefreshCache()
         ForcePageReset()
      EndEvent
      Event OnDefaultST()
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutEquSys_Desc_ActorSelect")
      EndEvent
   EndState
   State OPT_QuickslotsEnabled
      Event OnSelectST()
         SkyOutEquSysQuickslotManager kQM = GetQuickslotManager()
         kQM.SetEnabled(!kQM.GetEnabled())
         SetToggleOptionValueST(kQM.GetEnabled())
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutEquSys_Desc_EnableQuickslots")
      EndEvent
   EndState
   State OPT_PlayerInventoryManagementMode
      Event OnMenuOpenST()
         String[] sMenu = new String[3]
         sMenu[0] = "$SkyOutEquSys_OEdit_AddCancel"
         sMenu[1] = InventoryModeToTrString(1)
         sMenu[2] = InventoryModeToTrString(2)

         SetMenuDialogOptions(sMenu)
         SetMenuDialogStartIndex(_PlayerInventoryManagementMode)
         SetMenuDialogDefaultIndex(_PlayerInventoryManagementMode)
      EndEvent
      Event OnMenuAcceptST(Int aiIndex)
         If aiIndex == 0
            return
         ElseIf aiIndex <= 2
            SkyrimOutfitEquipmentSystemNativeFuncs.SetPlayerInventoryManagementMode(aiIndex)
            _PlayerInventoryManagementMode = aiIndex
         Endif

         SetMenuOptionValueST(InventoryModeToTrString(_PlayerInventoryManagementMode))
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutEquSys_Desc_InventoryManagementMode")
      EndEvent
   EndState

   State OPT_NPCInventoryManagementMode
      Event OnMenuOpenST()
         String[] sMenu = new String[3]
         sMenu[0] = "$SkyOutEquSys_OEdit_AddCancel"
         sMenu[1] = InventoryModeToTrString(1)
         sMenu[2] = InventoryModeToTrString(2)

         SetMenuDialogOptions(sMenu)
         SetMenuDialogStartIndex(_NPCInventoryManagementMode)
         SetMenuDialogDefaultIndex(_NPCInventoryManagementMode)
      EndEvent
      Event OnMenuAcceptST(Int aiIndex)
         If aiIndex == 0
            return
         ElseIf aiIndex <= 2
            SkyrimOutfitEquipmentSystemNativeFuncs.SetNPCInventoryManagementMode(aiIndex)
            _NPCInventoryManagementMode = aiIndex
         Endif

         SetMenuOptionValueST(InventoryModeToTrString(_NPCInventoryManagementMode))
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutEquSys_Desc_InventoryManagementMode")
      EndEvent
   EndState

   State OPT_ImportSettings
      Event OnSelectST()
         SkyrimOutfitEquipmentSystemNativeFuncs.ImportSettings()
         FullRefresh()
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutEquSys_Desc_Import")
      EndEvent
   EndState
   State OPT_ExportSettings
      Event OnSelectST()
         SkyrimOutfitEquipmentSystemNativeFuncs.ExportSettings()
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutEquSys_Desc_Export")
      EndEvent
   EndState
;/EndBlock/;

;/Block/; ; Outfit editing
   Function StartEditingOutfit(String asOutfitName)
      _sEditingOutfit = asOutfitName
      SetupSlotDataForOutfit(_sEditingOutfit)
      _iOutfitEditorBodySlotPage = 0
      _sOutfitShowingSlotEditor = False
      ForcePageReset()
   EndFunction
   Function StopEditingOutfit()
      _sOutfitShowingContextMenu = _sEditingOutfit
      _sEditingOutfit = ""
      ForcePageReset()
   EndFunction
   
   ;/Block/; ; Outfit browser
      Function ShowOutfitList()
         SetCursorFillMode(TOP_TO_BOTTOM)
         ;/Block/; ; Left column
            SetCursorPosition(0)
            AddHeaderOption("$SkyOutEquSys_MCMHeader_OutfitList")

            Int iCount
            Int iPageCount
            Int iFlagsPrev
            Int iFlagsNext
            
            If _sOutfitNames.Length > _OutfitsPageMaxOutfits ; too many to fit on one screen
               iCount     = _sOutfitNames.Length
               iPageCount = iCount / _OutfitsPageMaxOutfits
               If iPageCount * _OutfitsPageMaxOutfits < iCount
                  iPageCount = iPageCount + 1
               EndIf
               If _iOutfitBrowserPage >= iPageCount
                  _iOutfitBrowserPage = iPageCount - 1
               EndIf
               Int iOffset    = _iOutfitBrowserPage * _OutfitsPageMaxOutfits
               Int iIterator  = 0
               Int iMax       = iCount - iOffset
               If iMax > _OutfitsPageMaxOutfits
                  iMax = _OutfitsPageMaxOutfits
               EndIf
               While iIterator < iMax
                  String sName = _sOutfitNames[iIterator + iOffset]
                  String sMark = " "
                  If sName == _sSelectedOutfit
                     sMark = "$SkyOutEquSys_OutfitBrowser_ActiveMark"
                     If sName == _sOutfitShowingContextMenu
                        sMark = "$SkyOutEquSys_OutfitBrowser_ContextActiveMark"
                     EndIf
                  ElseIf sName == _sOutfitShowingContextMenu
                     sMark = "$SkyOutEquSys_OutfitBrowser_ContextMark"
                  EndIf
                  AddTextOptionST("OutfitList_Item_" + sName, sName, sMark)
                  iIterator = iIterator + 1
               EndWhile
               iFlagsPrev = OPTION_FLAG_NONE
               iFlagsNext = OPTION_FLAG_NONE
               If _iOutfitBrowserPage < 1
                  iFlagsPrev = OPTION_FLAG_DISABLED
               EndIf
               If _iOutfitBrowserPage == iPageCount - 1
                  iFlagsNext = OPTION_FLAG_DISABLED
               EndIf               
            Else
               Int iIterator = 0
               While iIterator < _sOutfitNames.Length
                  String sName = _sOutfitNames[iIterator]
                  String sMark = " "
                  If sName == _sSelectedOutfit
                     sMark = "$SkyOutEquSys_OutfitBrowser_ActiveMark"
                     If sName == _sOutfitShowingContextMenu
                        sMark = "$SkyOutEquSys_OutfitBrowser_ContextActiveMark"
                     EndIf
                  ElseIf sName == _sOutfitShowingContextMenu
                     sMark = "$SkyOutEquSys_OutfitBrowser_ContextMark"
                  EndIf
                  AddTextOptionST("OutfitList_Item_" + sName, sName, sMark)
                  iIterator = iIterator + 1
               EndWhile
            EndIf
         ;/EndBlock/;
         ;/Block/; ; Right column
            SetCursorPosition(1)

            If _sOutfitNames.Length > _OutfitsPageMaxOutfits
               AddHeaderOption("")
               AddTextOptionST("OutfitBrowser_Prev", "$SkyOutEquSys_MCMText_OutfitListPageNumber{" + (_iOutfitBrowserPage + 1) + "}{" + iPageCount + "}", "$SkyOutEquSys_MCMText_OutfitListButtonPagePrev", iFlagsPrev)
               AddTextOptionST("OutfitBrowser_Next", "", "$SkyOutEquSys_MCMText_OutfitListButtonPageNext", iFlagsNext)
            EndIf

            AddHeaderOption("$SkyOutEquSys_MCMHeader_GeneralActions")
            AddInputOptionST("OutfitContext_New", "$SkyOutEquSys_OContext_New", "")
            AddInputOptionST("OutfitContext_NewFromWorn", "$SkyOutEquSys_OContext_NewFromWorn", "")

            AddMenuOptionST("OutfitContext_SelectImportMod", "$SkyOutEquSys_OContext_SelectImportMod", _sOutfitImporter_SelectedMod)
            AddMenuOptionST("OutfitContext_ImportOutfitFromMod", "$SkyOutEquSys_OContext_ImportOutfitsFromMod", "")
            ;
            Int iContextFlags = OPTION_FLAG_HIDDEN
            If _sOutfitShowingContextMenu
               iContextFlags = OPTION_FLAG_NONE
            EndIf
            AddHeaderOption("$SkyOutEquSys_MCMHeader_OutfitActions{" + _sOutfitShowingContextMenu + "}", iContextFlags)
            If _sSelectedOutfit == _sOutfitShowingContextMenu
               AddTextOptionST("OutfitContext_Toggle", "$SkyOutEquSys_OContext_ToggleOff", "", iContextFlags)
            Else
               AddTextOptionST("OutfitContext_Toggle", "$SkyOutEquSys_OContext_ToggleOn", "", iContextFlags)
            EndIf
            ; TODO MINOR BUG: Emits warning when no outfit is selected, esp when there are no outfits in the list.
            If SkyrimOutfitEquipmentSystemNativeFuncs.GetOutfitFavoriteStatus(_sOutfitShowingContextMenu)
               AddTextOptionST("OutfitContext_Favorite", "$SkyOutEquSys_OContext_ToggleFavoriteOff", "", iContextFlags)
            Else
               AddTextOptionST("OutfitContext_Favorite", "$SkyOutEquSys_OContext_ToggleFavoriteOn", "", iContextFlags)
            EndIf
            AddTextOptionST ("OutfitContext_Edit",   "$SkyOutEquSys_OContext_Edit",   "", iContextFlags)
            AddInputOptionST("OutfitContext_Rename", "$SkyOutEquSys_OContext_Rename", "", iContextFlags)
            AddTextOptionST ("OutfitContext_Delete", "$SkyOutEquSys_OContext_Delete", "", iContextFlags)
         ;/EndBlock/;
      EndFunction
      State OutfitBrowser_Prev
         Event OnSelectST()
            _iOutfitBrowserPage = _iOutfitBrowserPage - 1
            ForcePageReset()
         EndEvent
      EndState
      State OutfitBrowser_Next
         Event OnSelectST()
            _iOutfitBrowserPage = _iOutfitBrowserPage + 1
            ForcePageReset()
         EndEvent
      EndState
      State OutfitContext_New
         Event OnInputOpenST()
            SetInputDialogStartText("outfit name or blank to cancel")
         EndEvent
         Event OnInputAcceptST(String asTextEntry)
            If !asTextEntry
               Return
            EndIf
            If StringUtil.GetLength(asTextEntry) > _iOutfitNameMaxBytes
               ShowMessage("$SkyOutEquSys_Err_OutfitNameTooLong", False, "$SkyOutEquSys_ErrDismiss")
               Return
            EndIf
            If SkyrimOutfitEquipmentSystemNativeFuncs.OutfitExists(asTextEntry)
               ShowMessage("$SkyOutEquSys_Err_OutfitNameTaken", False, "$SkyOutEquSys_ErrDismiss")
               Return
            EndIf
            SkyrimOutfitEquipmentSystemNativeFuncs.CreateOutfit(asTextEntry)
            RefreshCache()
            StartEditingOutfit(asTextEntry)
         EndEvent
      EndState
      State OutfitContext_NewFromWorn
         Event OnInputOpenST()
            SetInputDialogStartText("outfit name or blank to cancel")
         EndEvent
         Event OnInputAcceptST(String asTextEntry)
            If !asTextEntry
               Return
            EndIf
            If StringUtil.GetLength(asTextEntry) > _iOutfitNameMaxBytes
               ShowMessage("$SkyOutEquSys_Err_OutfitNameTooLong", False, "$SkyOutEquSys_ErrDismiss")
               Return
            EndIf
            If SkyrimOutfitEquipmentSystemNativeFuncs.OutfitExists(asTextEntry)
               ShowMessage("$SkyOutEquSys_Err_OutfitNameTaken", False, "$SkyOutEquSys_ErrDismiss")
               Return
            EndIf
            Armor[] kList = SkyrimOutfitEquipmentSystemNativeFuncs.GetWornItems(Game.GetPlayer())
            SkyrimOutfitEquipmentSystemNativeFuncs.OverwriteOutfit(asTextEntry, kList)
            RefreshCache()
            StartEditingOutfit(asTextEntry)
         EndEvent
      EndState

      State OutfitContext_SelectImportMod
         Event OnMenuOpenST()
            ; Get the paginated mod list
            Int totalMods = _sOutfitImporter_ModList.Length
            Int iTotalModPages = (totalMods + _iSelectMenuMax - 1) / _iSelectMenuMax
            
            ; Calculate start and end indices for the current page
            Int startIndex = (_OutfitImportModPage - 1) * _iSelectMenuMax
            
            ; Calculate end index with a simple comparison to avoid going past the array bounds
            Int maxEndIndex = totalMods - 1
            Int calculatedEndIndex = startIndex + _iSelectMenuMax - 1
            Int endIndex = calculatedEndIndex
            
            ; Ensure we don't exceed the array bounds
            If endIndex > maxEndIndex
                endIndex = maxEndIndex
            EndIf
            
            ; Calculate how many items will be on this page
            Int itemsOnPage = endIndex - startIndex + 1
            
            ; Add navigation buttons conditionally
            Bool hasPrevPage = (_OutfitImportModPage > 1) && iTotalModPages > 1
            Bool hasNextPage = (_OutfitImportModPage < iTotalModPages) && iTotalModPages > 1
            
            Int navOptionsCount = 0
            If hasPrevPage
                navOptionsCount += 1
            EndIf
            If hasNextPage
                navOptionsCount += 1
            EndIf
            
            ; Calculate total menu size: cancel button + nav options + mod entries
            Int menuSize = 1 + navOptionsCount + itemsOnPage
            
            ; Create menu array with exact size
            String[] sMenu = Utility.CreateStringArray(menuSize)
            sMenu[0] = "$SkyOutEquSys_OEdit_AddCancel" ; First option is always cancel
            
            ; Current position in the menu array
            Int menuIndex = 1
            
            ; Add navigation buttons
            If hasPrevPage
                sMenu[menuIndex] = "$SkyOutEquSys_MCMText_PrevPageOption"
                menuIndex += 1
            EndIf
            
            If hasNextPage
                sMenu[menuIndex] = "$SkyOutEquSys_MCMText_NextPageOption"
                menuIndex += 1
            EndIf
            
            ; Store the number of navigation options (cancel + prev/next) for later reference
            Int headerOptionsCount = 1 + navOptionsCount
            
            ; Add mod entries for the current page
            Int i = 0
            While i < itemsOnPage
                sMenu[menuIndex] = _sOutfitImporter_ModList[startIndex + i]
                menuIndex += 1
                i += 1
            EndWhile
            
            ; Set the menu options
            SetMenuDialogOptions(sMenu)
            SetMenuDialogStartIndex(0)
            SetMenuDialogDefaultIndex(0)
            
            ; Store the header count and page start index for the OnAccept handler
            _iOutfitImporter_HeaderOptionCount = headerOptionsCount
            _iOutfitImporter_PageStartIndex = startIndex
        EndEvent
         
         Event OnMenuAcceptST(Int aiIndex)
            Int iTotalModPages = (_sOutfitImporter_ModList.Length + _iSelectMenuMax - 1) / _iSelectMenuMax

            ; Get navigation state
            Bool hasPrevPage = (_OutfitImportModPage > 1) && iTotalModPages > 1
            Bool hasNextPage = (_OutfitImportModPage < iTotalModPages) && iTotalModPages > 1
            
            ; Handle user selection
            If aiIndex == 0 ; Cancel option
               Return
            EndIf
            
            ; Handle navigation options
            Int navOffset = 1 ; Start with 1 to account for the Cancel option
            
            ; Check if Previous Page was selected
            If hasPrevPage && aiIndex == navOffset
               _OutfitImportModPage -= 1
               SetMenuDialogOptions(new String[1]) ; Force menu to reopen
               ShowMessage("$SkyOutEquSys_MCMText_PrevPageOptionMessage", False, "$SkyOutEquSys_MessageDismiss")
               Return
            EndIf
            navOffset += hasPrevPage as Int ; Move offset if we have a prev button
            
            ; Check if Next Page was selected
            If hasNextPage && aiIndex == navOffset
               _OutfitImportModPage += 1
               SetMenuDialogOptions(new String[1]) ; Force menu to reopen
               ShowMessage("$SkyOutEquSys_MCMText_NextPageOptionMessage", False, "$SkyOutEquSys_MessageDismiss")
               Return
            EndIf
            
            ; If we get here, a mod was selected - adjust index to account for the header options
            Int modIndex = aiIndex - _iOutfitImporter_HeaderOptionCount + _iOutfitImporter_PageStartIndex
            If modIndex >= 0 && modIndex < _sOutfitImporter_ModList.Length
               _sOutfitImporter_SelectedMod = _sOutfitImporter_ModList[modIndex]
            EndIf
            
            ; Clear the candidates list when we're done
            SetMenuOptionValueST(_sOutfitImporter_SelectedMod)

            ; Add the candidates
            _sOutfitImporter_AddOutfitsForModCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.GetAllLoadedOutfitsForMod(_sOutfitImporter_SelectedMod)
         EndEvent
         
         Event OnHighlightST()                        
            ; Add page info to the highlight text
            Int iTotalModPages = (_sOutfitImporter_ModList.Length + _iSelectMenuMax - 1) / _iSelectMenuMax
            SetInfoText("$SkyOutEquSys_OContext_SelectImportModHighlight{" + _OutfitImportModPage + "}{" + iTotalModPages + "}{"+ _sOutfitImporter_ModList.Length + "}")
         EndEvent
      EndState     

      State OutfitContext_ImportOutfitFromMod
         Event OnMenuOpenST()
            ; Calculate total outfits and pages for the selected mod
            Int totalOutfits = _sOutfitImporter_AddOutfitsForModCandidates.Length
            Int _iTotalOutfitForModPages = (totalOutfits + _iSelectMenuMax - 1) / _iSelectMenuMax
            
            ; Calculate start and end indices for the current page
            Int startIndex = (_OutfitImportOutfitsForModPage - 1) * _iSelectMenuMax
            
            ; Calculate end index with a simple comparison
            Int maxEndIndex = totalOutfits - 1
            Int calculatedEndIndex = startIndex + _iSelectMenuMax - 1
            Int endIndex = calculatedEndIndex
            
            ; Ensure we don't exceed the array bounds
            If endIndex > maxEndIndex
                endIndex = maxEndIndex
            EndIf
            
            ; Calculate how many items will be on this page
            Int itemsOnPage = endIndex - startIndex + 1
            
            ; Add navigation buttons conditionally
            Bool hasPrevPage = (_OutfitImportOutfitsForModPage > 1) && _iTotalOutfitForModPages > 1
            Bool hasNextPage = (_OutfitImportOutfitsForModPage < _iTotalOutfitForModPages) && _iTotalOutfitForModPages > 1
            
            ; Count navigation options - start with 1 for the "Load All Outfits" option
            Int navOptionsCount = 1 
            
            If hasPrevPage
                navOptionsCount += 1
            EndIf
            If hasNextPage
                navOptionsCount += 1
            EndIf
            
            ; Calculate total menu size: cancel button + load all button + nav options + outfit entries for the page
            Int menuSize = 1 + 1 + (navOptionsCount - 1) + itemsOnPage
            
            ; Create menu array with exact size
            String[] sMenu = Utility.CreateStringArray(menuSize)
            
            ; Set cancel option
            sMenu[0] = "$SkyOutEquSys_OEdit_AddCancel"
            
            ; Set Load All Outfits option
            If totalOutfits > 0
               sMenu[1] = "$SkyOutEquSys_OEdit_LoadAllOutfits"
            Else 
               sMenu[1] = "$SkyOutEquSys_OEdit_NoOutfitsToLoad"
            EndIf 
            
            ; Current position in the menu array
            Int menuIndex = 2
            
            ; Add navigation buttons
            If hasPrevPage
                sMenu[menuIndex] = "$SkyOutEquSys_MCMText_PrevPageOption"
                menuIndex += 1
            EndIf
            
            If hasNextPage
                sMenu[menuIndex] = "$SkyOutEquSys_MCMText_NextPageOption"
                menuIndex += 1
            EndIf
            
            ; Store the number of navigation options (cancel + load all + prev/next) for later reference
            Int headerOptionsCount = 1 + navOptionsCount
            
            ; Add outfit entries for the current page
            Int i = 0
            While i < itemsOnPage
                sMenu[menuIndex] = _sOutfitImporter_AddOutfitsForModCandidates[startIndex + i]
                menuIndex += 1
                i += 1
            EndWhile
            
            ; Set the menu options
            SetMenuDialogOptions(sMenu)
            SetMenuDialogStartIndex(0)
            SetMenuDialogDefaultIndex(0)
            
            ; Store the header count and page start index for the OnAccept handler
            _iOutfitForModImporter_HeaderOptionCount = headerOptionsCount
            _iOutfitForModImporter_PageStartIndex = startIndex
        EndEvent
          
         Event OnMenuAcceptST(Int aiIndex)
             ; Get navigation state
             Int _iTotalOutfitForModPages = (_sOutfitImporter_AddOutfitsForModCandidates.Length + _iSelectMenuMax - 1) / _iSelectMenuMax
             Bool hasPrevPage = (_OutfitImportOutfitsForModPage > 1) && _iTotalOutfitForModPages > 1
             Bool hasNextPage = (_OutfitImportOutfitsForModPage < _iTotalOutfitForModPages) && _iTotalOutfitForModPages > 1
             
             ; Handle user selection
             If aiIndex == 0 ; Cancel option
                 Return
             EndIf
             
             ; Check if "Load All Outfits" option was selected (index 1)
             If aiIndex == 1
                 ; Use the native function to add all outfits from the mod
                 Int addedCount = SkyrimOutfitEquipmentSystemNativeFuncs.AddAllOutfitsFromModToOutfitList(_sOutfitImporter_SelectedMod)
                 
                 If addedCount > 0
                     ShowMessage("$SkyOutEquSys_OContext_ImportAllOutfitsFromMod_Success{" + addedCount + "}{" + _sOutfitImporter_SelectedMod + "}", False, "$SkyOutEquSys_MessageDismiss")
                     FullRefresh()
                 Else
                     ShowMessage("$SkyOutEquSys_OContext_ImportAllOutfitsFromMod_Failure{" + _sOutfitImporter_SelectedMod + "}", False, "$SkyOutEquSys_MessageDismiss")
                 EndIf
                 
                 Return
             EndIf
             
             ; Handle navigation options
             Int navOffset = 2 ; Start with 2 to account for Cancel and Load All options
             
             ; Check if Previous Page was selected
             If hasPrevPage && aiIndex == navOffset
                 _OutfitImportOutfitsForModPage -= 1
                 SetMenuDialogOptions(new String[1]) ; Force menu to reopen
                 ShowMessage("$SkyOutEquSys_MCMText_PrevPageOptionMessage", False, "$SkyOutEquSys_MessageDismiss")
                 Return
             EndIf
             navOffset += hasPrevPage as Int ; Move offset if we have a prev button
             
             ; Check if Next Page was selected
             If hasNextPage && aiIndex == navOffset
                 _OutfitImportOutfitsForModPage += 1
                 SetMenuDialogOptions(new String[1]) ; Force menu to reopen
                 ShowMessage("$SkyOutEquSys_MCMText_NextPageOptionMessage", False, "$SkyOutEquSys_MessageDismiss")
                 Return
             EndIf
             
             ; If we get here, an outfit was selected - adjust index to account for the header options
             Int outfitIndex = aiIndex - _iOutfitForModImporter_HeaderOptionCount + _iOutfitForModImporter_PageStartIndex
             Int OutfitAddStatus = 0
             String SelectedOutfitEditorFormID = ""
     
             If outfitIndex >= 0 && outfitIndex < _sOutfitImporter_AddOutfitsForModCandidates.Length
                 ; Here handle the selected outfit
                 SelectedOutfitEditorFormID = _sOutfitImporter_AddOutfitsForModCandidates[outfitIndex]
                 SetMenuOptionValueST(SelectedOutfitEditorFormID)
                 OutfitAddStatus = SkyrimOutfitEquipmentSystemNativeFuncs.AddOutfitFromModToOutfitList(_sOutfitImporter_SelectedMod, SelectedOutfitEditorFormID)
                 
                 If OutfitAddStatus == 1
                     ShowMessage("$SkyOutEquSys_OContext_ImportOutfitsFromMod_Success{"+SelectedOutfitEditorFormID+"}{"+_sOutfitImporter_SelectedMod+"}", False, "$SkyOutEquSys_MessageDismiss")
                     FullRefresh()
                 Else 
                     ShowMessage("$SkyOutEquSys_OContext_ImportOutfitsFromMod_Failure{"+SelectedOutfitEditorFormID+"}{"+_sOutfitImporter_SelectedMod+"}", False, "$SkyOutEquSys_MessageDismiss")
                 EndIf
             EndIf
         EndEvent
          
         Event OnHighlightST()                        
             ; Add page info to the highlight text
             Int _iTotalOutfitForModPages = (_sOutfitImporter_AddOutfitsForModCandidates.Length + _iSelectMenuMax - 1) / _iSelectMenuMax

             If _sOutfitImporter_SelectedMod == ""
                 SetInfoText("$SkyOutEquSys_OContext_ImportOutfitsFromMod")
             Else 
                 SetInfoText("$SkyOutEquSys_OContext_ImportOutfitsFromModHighlight{" + _sOutfitImporter_SelectedMod + "}{"+ _OutfitImportOutfitsForModPage + "}{" + _iTotalOutfitForModPages + "}{"+ _sOutfitImporter_AddOutfitsForModCandidates.Length + "}")
             EndIf
         EndEvent
     EndState

      State OutfitContext_Toggle
         Event OnSelectST()
            If _sSelectedOutfit == _sOutfitShowingContextMenu
               SkyrimOutfitEquipmentSystemNativeFuncs.SetSelectedOutfit(_aCurrentActor, "")
            Else
               SkyrimOutfitEquipmentSystemNativeFuncs.SetSelectedOutfit(_aCurrentActor, _sOutfitShowingContextMenu)
            EndIf
            RefreshCache()
            ForcePageReset()
         EndEvent
      EndState
      State OutfitContext_Favorite
         Event OnSelectST()
            If SkyrimOutfitEquipmentSystemNativeFuncs.GetOutfitFavoriteStatus(_sOutfitShowingContextMenu)
               SkyrimOutfitEquipmentSystemNativeFuncs.SetOutfitFavoriteStatus(_sOutfitShowingContextMenu, false)
            Else
               SkyrimOutfitEquipmentSystemNativeFuncs.SetOutfitFavoriteStatus(_sOutfitShowingContextMenu, true)
            EndIf
            RefreshCache()
            ForcePageReset()
         EndEvent
      EndState
      State OutfitContext_Edit
         Event OnSelectST()
            StartEditingOutfit(_sOutfitShowingContextMenu)
         EndEvent
      EndState
      State OutfitContext_Rename
         Event OnInputOpenST()
            SetInputDialogStartText("outfit name or blank to cancel")
         EndEvent
         Event OnInputAcceptST(String asTextEntry)
            If !asTextEntry
               Return
            EndIf
            If asTextEntry == _sOutfitShowingContextMenu
               Return
            EndIf
            If StringUtil.GetLength(asTextEntry) > _iOutfitNameMaxBytes
               ShowMessage("$SkyOutEquSys_Err_OutfitNameTooLong", False, "$SkyOutEquSys_ErrDismiss")
               Return
            EndIf
            If SkyrimOutfitEquipmentSystemNativeFuncs.OutfitExists(asTextEntry)
               ShowMessage("$SkyOutEquSys_Err_OutfitNameTaken", False, "$SkyOutEquSys_ErrDismiss")
               Return
            EndIf
            Bool bSuccess = SkyrimOutfitEquipmentSystemNativeFuncs.RenameOutfit(_sOutfitShowingContextMenu, asTextEntry)
            If bSuccess
               _sOutfitShowingContextMenu = asTextEntry
               RefreshCache()
               ForcePageReset()
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutEquSys_MCMInfoText_RenameOutfit{" + _sOutfitShowingContextMenu + "}")
         EndEvent
      EndState
      State OutfitContext_Delete
         Event OnSelectST()
            If !_sOutfitShowingContextMenu
               Return
            EndIf
            Bool bDelete = ShowMessage("$SkyOutEquSys_Confirm_Delete_Text{" + _sOutfitShowingContextMenu + "}", True, "$SkyOutEquSys_Confirm_Delete_Yes", "$SkyOutEquSys_Confirm_Delete_No")
            If bDelete
               SkyrimOutfitEquipmentSystemNativeFuncs.DeleteOutfit(_sOutfitShowingContextMenu)
               ;
               RefreshCache()
               StopEditingOutfit()
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutEquSys_MCMInfoText_DeleteOutfit{" + _sOutfitShowingContextMenu + "}")
         EndEvent
      EndState
   ;/EndBlock/;
   ;/Block/; ; Outfit editor
      Function ShowOutfitEditor()
         SetCursorFillMode(TOP_TO_BOTTOM)
         ;/Block/; ; Left column
            SetCursorPosition(0)
            AddHeaderOption ("$SkyOutEquSys_MCMHeader_OutfitEditor{" + _sEditingOutfit + "}")
            AddTextOptionST ("OutfitEditor_Back",           "$SkyOutEquSys_OEdit_Back", "")
            AddMenuOptionST ("OutfitEditor_AddFromCarried", "$SkyOutEquSys_OEdit_AddFromCarried", "")
            AddMenuOptionST ("OutfitEditor_AddFromWorn",    "$SkyOutEquSys_OEdit_AddFromWorn", "")
            AddInputOptionST("OutfitEditor_AddByID",        "$SkyOutEquSys_OEdit_AddByID", "")
            ; AddEmptyOption()
            AddHeaderOption  ("$SkyOutEquSys_OEdit_AddFromList_Header")
            AddMenuOptionST  ("OutfitEditor_AddFromList_Menu",     "$SkyOutEquSys_OEdit_AddFromList_Search", "")
            AddInputOptionST ("OutfitEditor_AddFromList_Filter",   "$SkyOutEquSys_OEdit_AddFromList_Filter_Name", _sOutfitEditor_AddFromList_Filter)
            AddToggleOptionST("OutfitEditor_AddFromList_Playable", "$SkyOutEquSys_OEdit_AddFromList_Filter_Playable", _bOutfitEditor_AddFromList_Playable)

            If !_sOutfitShowingSlotEditor
               ShowOutfitSlots()
            EndIf
            ;
            ; All add functions must fail if the armor already exists 
            ; in the item (though that shouldn't cause problems on the 
            ; DLL side of things; we use std::set rather than vector, 
            ; so redundant entries are impossible anyway).
            ;
         ;/EndBlock/;
      EndFunction
      Function ShowOutfitSlots()
         ;/Block/; ; Right column
         SetCursorPosition(1)
         AddHeaderOption("$SkyOutEquSys_MCMHeader_OutfitSlots")
         ;
         ; The goal here:
         ;
         ;  - Show only the body slots that the outfit uses
         ;
         ;  - If a body slot is covered by multiple different armors 
         ;    in the outfit (i.e. the user has chosen to enable the 
         ;    use of conflicting armors), then show the slot multiple 
         ;    times, once for each armor.
         ;
         ;     - As of this writing, conflicting armors don't actually 
         ;       work in our patch; the last armor for a slot "wins." 
         ;       However, there are no real engine limitations on how 
         ;       many ArmorAddons can cover a given body part; if we 
         ;       were to reconfigure our patch, then we could allow 
         ;       users to enable conflicts on a per-armor basis. As 
         ;       such, this approach is prep work for that.
         ;
         ;       (During early development, it wasn't immediately 
         ;       clear whether our patch supported conflicting slots; 
         ;       random tinkering in the R&D stage proved that the 
         ;       innermost bits of the armor system allow conflicts, 
         ;       but I didn't know whether my patch was low-level 
         ;       enough to take advantage of that. This approach to 
         ;       listing the outfit contents was designed at that 
         ;       stage of development.)
         ;
         Int iSlotCount = _sOutfitSlotNames.Length
         If iSlotCount > 11
            Int iPageCount = iSlotCount / 8
            If iPageCount * 8 < iSlotCount
               iPageCount = iPageCount + 1
            EndIf
            If _iOutfitEditorBodySlotPage >= iPageCount
               _iOutfitEditorBodySlotPage = iPageCount - 1
            EndIf
            Int iOffset    = _iOutfitEditorBodySlotPage * 8
            Int iIterator  = 0
            Int iMax       = iSlotCount - iOffset
            If iMax > 8 ; (visible - 3) -- make room for page separator and buttons
               iMax = 8
            EndIf
            While iIterator < iMax
               String sSlot  = _sOutfitSlotNames [iIterator + iOffset]
               String sArmor = _sOutfitSlotArmors[iIterator + iOffset]
               If !sArmor
                  sArmor = "$SkyOutEquSys_NamelessArmor"
               EndIf
               AddTextOptionST("OutfitEditor_BodySlot_" + iIterator, sSlot, sArmor)
               iIterator = iIterator + 1
            EndWhile
            Int iFlagsPrev = OPTION_FLAG_NONE
            Int iFlagsNext = OPTION_FLAG_NONE
            If _iOutfitEditorBodySlotPage < 1
               iFlagsPrev = OPTION_FLAG_DISABLED
            EndIf
            If _iOutfitEditorBodySlotPage == iPageCount - 1
               iFlagsNext = OPTION_FLAG_DISABLED
            EndIf
            SetCursorPosition(19)
            AddHeaderOption("")
            AddTextOptionST("OutfitEditor_BodySlotsPrev", "$SkyOutEquSys_MCMText_OutfitSlotsPageNumber{" + (_iOutfitEditorBodySlotPage + 1) + "}{" + iPageCount + "}", "$SkyOutEquSys_MCMText_OutfitSlotsButtonPagePrev", iFlagsPrev)
            AddTextOptionST("OutfitEditor_BodySlotsNext", "", "$SkyOutEquSys_MCMText_OutfitSlotsButtonPageNext", iFlagsNext)
         ElseIf iSlotCount
            Int iIterator = 0
            While iIterator < iSlotCount
               String sSlot  = _sOutfitSlotNames[iIterator]
               String sArmor = _sOutfitSlotArmors[iIterator]
               If !sArmor
                  sArmor = "$SkyOutEquSys_NamelessArmor"
               EndIf
               AddTextOptionST("OutfitEditor_BodySlot_" + iIterator, sSlot, sArmor)
               iIterator = iIterator + 1
            EndWhile
         Else
            AddTextOption("$SkyOutEquSys_OutfitEditor_OutfitIsEmpty", "")
         EndIf
      ;/EndBlock/;
      EndFunction
      Function AddArmorToOutfit(Armor kAdd)
         If !kAdd || !_sEditingOutfit
            Return
         EndIf
         If SkyrimOutfitEquipmentSystemNativeFuncs.ArmorConflictsWithOutfit(kAdd, _sEditingOutfit)
            Bool bSwap = ShowMessage("$SkyOutEquSys_Confirm_BodySlotConflict_Text", True, "$SkyOutEquSys_Confirm_BodySlotConflict_Yes", "$SkyOutEquSys_Confirm_BodySlotConflict_No")
            If bSwap
               SkyrimOutfitEquipmentSystemNativeFuncs.RemoveConflictingArmorsFrom(kAdd, _sEditingOutfit)
            Else
               Return
            EndIf
         EndIf
         SkyrimOutfitEquipmentSystemNativeFuncs.AddArmorToOutfit(_sEditingOutfit, kAdd)
         SetupSlotDataForOutfit(_sEditingOutfit)
         ForcePageReset()
      EndFunction
      ;
      State OutfitEditor_BodySlotsPrev
         Event OnSelectST()
            _iOutfitEditorBodySlotPage = _iOutfitEditorBodySlotPage - 1
            ForcePageReset()
         EndEvent
      EndState
      State OutfitEditor_BodySlotsNext
         Event OnSelectST()
            _iOutfitEditorBodySlotPage = _iOutfitEditorBodySlotPage + 1
            ForcePageReset()
         EndEvent
      EndState
      ;
      State OutfitEditor_Back
         Event OnSelectST()
            StopEditingOutfit()
            ForcePageReset()
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutEquSys_MCMInfoText_BackToOutfitList")
         EndEvent
      EndState
      State OutfitEditor_AddFromCarried
         Event OnMenuOpenST()
            _kOutfitEditor_AddCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.GetCarriedArmor(Game.GetPlayer())
            Int iCount = _kOutfitEditor_AddCandidates.Length
            _sOutfitEditor_AddCandidates = Utility.CreateStringArray(iCount)
            Int iIterator = 0
            While iIterator < iCount
               Armor  kCurrent = _kOutfitEditor_AddCandidates[iIterator]
               String sCurrent = ""
               If kCurrent
                  sCurrent = kCurrent.GetName()
               EndIf
               If !sCurrent
                  sCurrent = "$SkyOutEquSys_NamelessArmor"
               EndIf
               _sOutfitEditor_AddCandidates[iIterator] = sCurrent
               iIterator = iIterator + 1
            EndWhile
            ;
            _kOutfitEditor_AddCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.NaturalSortPairArmor_ASCII(_sOutfitEditor_AddCandidates, _kOutfitEditor_AddCandidates)
            _sOutfitEditor_AddCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.NaturalSort_ASCII(_sOutfitEditor_AddCandidates)
            ;
            String[] sMenu = PrependStringToArray(_sOutfitEditor_AddCandidates, "$SkyOutEquSys_OEdit_AddCancel")
            ;
            SetMenuDialogOptions(sMenu)
            SetMenuDialogStartIndex(0)
            SetMenuDialogDefaultIndex(0)
         EndEvent
         Event OnMenuAcceptST(Int aiIndex)
            aiIndex = aiIndex - 1 ; first menu item is a "cancel" option
            If aiIndex < 0 ; user canceled
               _sOutfitEditor_AddCandidates = new String[1]
               _kOutfitEditor_AddCandidates = new Armor[1]
               Return
            EndIf
            Armor kCurrent = _kOutfitEditor_AddCandidates[aiIndex]
            _sOutfitEditor_AddCandidates = new String[1]
            _kOutfitEditor_AddCandidates = new Armor[1]
            If kCurrent
               AddArmorToOutfit(kCurrent)
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutEquSys_MCMInfoText_AddToOutfitFromCarried")
         EndEvent
      EndState
      State OutfitEditor_AddFromWorn
         Event OnMenuOpenST()
            _kOutfitEditor_AddCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.GetWornItems(Game.GetPlayer())
            Int iCount = _kOutfitEditor_AddCandidates.Length
            _sOutfitEditor_AddCandidates = Utility.CreateStringArray(iCount)
            Int iIterator = 0
            While iIterator < iCount
               Armor  kCurrent = _kOutfitEditor_AddCandidates[iIterator]
               String sCurrent = ""
               If kCurrent
                  sCurrent = kCurrent.GetName()
               EndIf
               If !sCurrent
                  sCurrent = "$SkyOutEquSys_NamelessArmor"
               EndIf
               _sOutfitEditor_AddCandidates[iIterator] = sCurrent
               iIterator = iIterator + 1
            EndWhile
            ;
            _kOutfitEditor_AddCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.NaturalSortPairArmor_ASCII(_sOutfitEditor_AddCandidates, _kOutfitEditor_AddCandidates)
            _sOutfitEditor_AddCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.NaturalSort_ASCII(_sOutfitEditor_AddCandidates)
            ;
            String[] sMenu = PrependStringToArray(_sOutfitEditor_AddCandidates, "$SkyOutEquSys_OEdit_AddCancel")
            ;
            SetMenuDialogOptions(sMenu)
            SetMenuDialogStartIndex(0)
            SetMenuDialogDefaultIndex(0)
         EndEvent
         Event OnMenuAcceptST(Int aiIndex)
            aiIndex = aiIndex - 1 ; first menu item is a "cancel" option
            If aiIndex < 0 ; user canceled
               _sOutfitEditor_AddCandidates = new String[1]
               _kOutfitEditor_AddCandidates = new Armor[1]
               Return
            EndIf
            Armor kCurrent = _kOutfitEditor_AddCandidates[aiIndex]
            _sOutfitEditor_AddCandidates = new String[1]
            _kOutfitEditor_AddCandidates = new Armor[1]
            If kCurrent
               AddArmorToOutfit(kCurrent)
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutEquSys_MCMInfoText_AddToOutfitFromWorn")
         EndEvent
      EndState
      State OutfitEditor_AddByID
         Event OnInputOpenST()
            SetInputDialogStartText("0x00000000")
         EndEvent
         Event OnInputAcceptST(String asTextEntry)
            If !asTextEntry
               Return
            EndIf
            Int iFormID = SkyrimOutfitEquipmentSystemNativeFuncs.HexToInt32(asTextEntry)
            If !iFormID
               Return
            EndIf
            Form  kForm  = Game.GetForm(iFormID)
            Armor kArmor = Game.GetForm(iFormID) as Armor
            If !kArmor
               If !kForm
                  ShowMessage("$SkyOutEquSys_Err_FormDoesNotExist", False, "$SkyOutEquSys_ErrDismiss")
                  Return
               EndIf
               ShowMessage("$SkyOutEquSys_Err_FormIsNotArmor", False, "$SkyOutEquSys_ErrDismiss")
               Return
            EndIf
            String sName = kArmor.GetName()
            If !sName
               sName = "$SkyOutEquSys_NamelessArmor"
            EndIf
            Bool bConfirm = ShowMessage("$SkyOutEquSys_Confirm_AddByID_Text{" + sName + "}", True, "$SkyOutEquSys_Confirm_AddByID_Yes", "$SkyOutEquSys_Confirm_AddByID_No")
            If bConfirm
               AddArmorToOutfit(kArmor)
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutEquSys_MCMInfoText_AddToOutfitByID")
         EndEvent
      EndState
      ;
      ;/Block/; ; Add-from-list
         Function UpdateArmorSearch()
            SkyrimOutfitEquipmentSystemNativeFuncs.PrepArmorSearch(_sOutfitEditor_AddFromList_Filter, _bOutfitEditor_AddFromList_Playable)
            _sOutfitEditor_AddFromListCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.GetArmorSearchResultNames()
            _kOutfitEditor_AddFromListCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.GetArmorSearchResultForms()
            SkyrimOutfitEquipmentSystemNativeFuncs.ClearArmorSearch()
            ;
            _kOutfitEditor_AddFromListCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.NaturalSortPairArmor_ASCII(_sOutfitEditor_AddFromListCandidates, _kOutfitEditor_AddFromListCandidates)
            _sOutfitEditor_AddFromListCandidates = SkyrimOutfitEquipmentSystemNativeFuncs.NaturalSort_ASCII(_sOutfitEditor_AddFromListCandidates)
         EndFunction
         ;
         State OutfitEditor_AddFromList_Menu
            Event OnMenuOpenST()
               UpdateArmorSearch()
               String[] sMenu = PrependStringToArray(_sOutfitEditor_AddFromListCandidates, "$SkyOutEquSys_OEdit_AddCancel")
               ;
               SetMenuDialogOptions(sMenu)
               SetMenuDialogStartIndex(0)
               SetMenuDialogDefaultIndex(0)
            EndEvent
            Event OnMenuAcceptST(Int aiIndex)
               aiIndex = aiIndex - 1 ; first menu item is a "cancel" option
               If aiIndex < 0 ; user canceled
                  Return
               EndIf
               Armor kCurrent = _kOutfitEditor_AddFromListCandidates[aiIndex]
               If kCurrent
                  AddArmorToOutfit(kCurrent)
               EndIf
            EndEvent
         EndState
         State OutfitEditor_AddFromList_Filter
            Event OnInputOpenST()
               SetInputDialogStartText(_sOutfitEditor_AddFromList_Filter)
            EndEvent
            Event OnInputAcceptST(String asTextEntry)
               If asTextEntry == _sOutfitEditor_AddFromList_Filter
                  Return
               EndIf
               _sOutfitEditor_AddFromList_Filter = asTextEntry
               SetInputOptionValueST(asTextEntry)
            EndEvent
         EndState
         State OutfitEditor_AddFromList_Playable
            Event OnSelectST()
               _bOutfitEditor_AddFromList_Playable = !_bOutfitEditor_AddFromList_Playable
               SetToggleOptionValueST(_bOutfitEditor_AddFromList_Playable)
            EndEvent
         EndState
      ;/EndBlock/;
   ;/EndBlock/;
;/EndBlock/;
