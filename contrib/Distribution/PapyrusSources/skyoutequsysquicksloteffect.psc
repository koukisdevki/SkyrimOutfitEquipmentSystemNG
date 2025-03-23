Scriptname SkyOutEquSysQuickslotEffect extends activemagiceffect

Event OnEffectStart(Actor akCaster, Actor akTarget)
   String[] sLMenuItems = SkyrimOutfitEquipmentSystemNativeFuncs.ListOutfits(favoritesOnly = true)
   sLMenuItems = SkyrimOutfitEquipmentSystemNativeFuncs.NaturalSort_ASCII(sLMenuItems)
   UIListMenu menu = UIExtensions.GetMenu("UIListMenu") as UIListMenu
   Int iIndex = 0
   menu.AddEntryItem("[DISMISS]")
   menu.AddEntryItem("[NO OUTFIT]")
   While iIndex < sLMenuItems.Length
      menu.AddEntryItem(sLMenuItems[iIndex])
      iIndex = iIndex + 1
   EndWhile
   UIExtensions.OpenMenu("UIListMenu")
   String result = menu.GetResultString()
   Debug.Trace("User selected outfit: " + result)
   If result == "" ; Cover the case where the user backs out of the menu
      result = "[DISMISS]"
   Endif
   If result == "[NO OUTFIT]"
      result = ""
   Endif
   If result != "[DISMISS]"
      SkyrimOutfitEquipmentSystemNativeFuncs.SetSelectedOutfit(Game.GetPlayer(), result)
      ; Update the autoswitch slot if
      ; 1) autoswitching is enabled,
      ; 2) the current location has an outfit assigned already, and
      ; 3) if we have an outfit selected in this menu
      Int playerLocationType = SkyrimOutfitEquipmentSystemNativeFuncs.IdentifyLocationType(Game.GetPlayer().GetCurrentLocation(), Weather.GetCurrentWeather(), Game.GetPlayer())
      If SkyrimOutfitEquipmentSystemNativeFuncs.GetLocationOutfit(Game.GetPlayer(), playerLocationType) != "" && result != ""
         SkyrimOutfitEquipmentSystemNativeFuncs.SetLocationOutfit(Game.GetPlayer(), playerLocationType, result)
         Debug.Notification("This outfit will be remembered for this location type.")
      EndIf
      SkyrimOutfitEquipmentSystemNativeFuncs.RefreshArmorFor(Game.GetPlayer())
   Endif
EndEvent
