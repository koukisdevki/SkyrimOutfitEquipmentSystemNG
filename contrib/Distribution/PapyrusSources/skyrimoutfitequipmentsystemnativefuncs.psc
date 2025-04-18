Scriptname SkyrimOutfitEquipmentSystemNativeFuncs Hidden

;
; Information on the outfit system:
;
;  - Outfit names are not sorted; functions to retrieve a list of outfits 
;    will return an unsorted list. List order will remain consistent until 
;    the list changes (i.e. the std::unordered_map's iterators invalidate).
;
;  - Inventory lists are unsorted but likely to remain similarly consistent.
;
;  - Armor search results are not intentionally sorted, but will most 
;    likely be returned in form ID order.
;
;  - Body part listings are sorted by body part index.
;
;  - Outfit names shouldn't be longer than the constant returned by a call 
;    to GetOutfitNameMaxLength(); currently this limit is due to how SKSE 
;    serializes and loads strings in the co-save.
;
;  - Outfit names shouldn't be blank; a blank outfit name refers to "not 
;    using an outfit."
;

Int Function GetOutfitNameMaxLength() Global Native

Armor[] Function GetCarriedArmor (Actor akSubject) Global Native
Armor[] Function GetWornItems    (Actor akSubject) Global Native
        Function RefreshArmorFor (Actor akSubject) Global Native ; force akSubject to update their ArmorAddons
        Function RefreshArmorForAllConfiguredActors () Global Native ; force all known actors to update their ArmorAddons
;
; Searching for actors. Used in menus.
;
Actor[] Function ActorsNearPC  (ObjectReference crosshairActor) Global Native

;
; Search through all armor forms defined in the game (excluding templated ones). 
; Filter by name or require the "playable" flag.
;
         Function PrepArmorSearch           (String asNameFilter = "", Bool abMustBePlayable = True) Global Native
Armor[]  Function GetArmorSearchResultForms () Global Native
String[] Function GetArmorSearchResultNames () Global Native
         Function ClearArmorSearch          () Global Native

;
; Given an outfit, generate string and form arrays representing which body slots 
; are taken by which armors.
;
         Function PrepOutfitBodySlotListing           (String asOutfitName) Global Native
Armor[]  Function GetOutfitBodySlotListingArmorForms  () Global Native
String[] Function GetOutfitBodySlotListingArmorNames  () Global Native
Int[]    Function GetOutfitBodySlotListingSlotIndices () Global Native
         Function ClearOutfitBodySlotListing          () Global Native

;
; String functions, to be synchronized with CobbAPI:
;
String[] Function NaturalSort_ASCII          (String[] asStrings, Bool abDescending = False) Global Native
Armor[]  Function NaturalSortPairArmor_ASCII (String[] asStrings, Armor[] akForms, Bool abDescending = False) Global Native ; uses asStrings to sort akForms
;
Int      Function HexToInt32 (String asHex) Global Native
String   Function ToHex      (Int aiValue, Int aiDigits) Global Native

;
; Functions for working with the armor-addon override service.
;
         Function AddArmorToOutfit  (String asOutfitName, Armor akArmor) Global Native
Bool     Function ArmorConflictsWithOutfit (Armor akTest, String asOutfitName) Global Native
         Function CreateOutfit      (String asOutfitName) Global Native
         Function DeleteOutfit      (String asOutfitName) Global Native
Armor[]  Function GetOutfitContents (String asOutfitName) Global Native
Bool     Function GetOutfitFavoriteStatus(String asOutfitName) Global Native
         Function SetOutfitFavoriteStatus(String asOutfitName, Bool abFavorite) Global Native
String[] Function GetAvailablePolicyNames() Global Native
String[] Function GetAvailablePolicyCodes() Global Native
String   Function GetSelectedOutfit (Actor actor) Global Native
Bool     Function IsEnabled         () Global Native
String[] Function ListOutfits       (Bool favoritesOnly = False) Global Native
         Function RemoveArmorFromOutfit (String asOutfitName, Armor akArmor) Global Native
         Function RemoveConflictingArmorsFrom (Armor akTest, String asOutfitName) Global Native
Bool     Function RenameOutfit      (String asOutfitName, String asRenameTo) Global Native
Bool     Function OutfitExists      (String asOutfitName) Global Native
         Function OverwriteOutfit   (String asOutfitName, Armor[] akArmors) Global Native
         Function SetEnabled        (Bool abEnabled) Global Native
         Function SetSelectedOutfit (Actor actor, String asOutfitName) Global Native
         Function AddActor (Actor akSubject) Global Native
         Function RemoveActor (Actor akSubject) Global Native
Actor[]  Function ListActors() Global Native
bool     Function HasActor(Actor target) Global Native

Int      Function GetPlayerInventoryManagementMode () Global Native
         Function SetPlayerInventoryManagementMode (Int mode) Global Native
Int      Function GetNPCInventoryManagementMode () Global Native
         Function SetNPCInventoryManagementMode (Int mode) Global Native

Int[]    Function GetAutoSwitchGenericLocationArray () Global Native
Int[]    Function GetAutoSwitchSpecificLocationArray () Global Native
Int[]    Function GetAutoSwitchActionBasedLocationArray () Global Native
Int      Function IdentifyLocationType (Location alLocation, Weather awWeather, Actor target) Global Native
         Function SetOutfitsUsingLocation (Location alLocation, Weather awWeather) Global Native
         Function SetLocationOutfit (Actor actor, Int aiLocationType, String asOutfitName) Global Native
         Function UnsetLocationOutfit (Actor actor, Int aiLocationType) Global Native
String   Function GetLocationOutfit (Actor actor, Int aiLocationType) Global Native
Bool     Function ExportSettings () Global Native
Bool     Function ImportSettings () Global Native

String[] Function GetAllLoadedOutfitModsList () Global Native
String[] Function GetAllLoadedOutfitsForMod (String modName) Global Native
String[] Function GetAllLoadedArmorModsList () Global Native
Armor[]  Function GetAllLoadedArmorsForMod (String modName) Global Native
String[] Function GetAllLoadedArmorsForModAsStrings (String modName) Global Native

Int      Function AddOutfitFromModToOutfitList(String modName, String formEditorID) Global Native
Int      Function AddAllOutfitsFromModToOutfitList(String modName) Global Native

         Function AutoOutfitSwitchStateReset () Global Native

         Function SetLoveSceneForActors(Actor[] actors) Global Native
         Function UnsetLoveSceneForActors(Actor[] actors) Global Native
         
Int      Function GetIniOptionValueFor(string setting) Global Native
String   Function GetStringOptionValueFor(string setting) Global Native
Bool     Function IsVRMode() Global Native

String   Function GenerateNewOutfitName() Global Native
String   Function GenerateOutfitNameForOutfit(string outfit) Global Native

bool     Function IsQuickslotEnabled () Global Native
         Function SetQuickslotEnabled (Bool abEnabled) Global Native
bool     Function IsClimatePriorityEnabled () Global Native
         Function SetClimatePriorityEnabled (Bool abEnabled) Global Native