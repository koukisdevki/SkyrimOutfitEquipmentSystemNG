Scriptname SkyOutEquSysOstim extends Quest  

OsexIntegrationMain ostim 

Event OnInit()
	ostim = OUtils.GetOStim()

	RegisterForModEvent("ostim_thread_start", "OStimThreadStart")
	RegisterForModEvent("ostim_thread_end", "OStimThreadEnd")
EndEvent
	
Event OStimThreadStart(string EventName, string StrArg, float ThreadID, Form Sender)
    int currentThreadID = ThreadID as Int
    Actor[] ActorArray = OThread.GetActors(currentThreadID)
	SkyrimOutfitEquipmentSystemNativeFuncs.SetLoveSceneForActors(ActorArray)
	
	; for some reason ostim preloads the player's clothes before the scene, so reset here. 
	; But its also good to do this for all cases for instant update
	If SkyrimOutfitEquipmentSystemNativeFuncs.IsEnabled()
		SkyrimOutfitEquipmentSystemNativeFuncs.SetOutfitsUsingLocation(Game.GetPlayer().GetCurrentLocation(), Weather.GetCurrentWeather())
		SkyrimOutfitEquipmentSystemNativeFuncs.RefreshArmorForAllConfiguredActors()
	Endif

	Debug.Notification("Starting OStim thread, exec SkyOutEquSysOstim.")
EndEvent

Event OStimThreadEnd(string EventName, string Json, float ThreadID, Form Sender)
	; the following code only works with API version 7.3.1 or higher
	Actor[] Actors = OJSON.GetActors(Json)
	SkyrimOutfitEquipmentSystemNativeFuncs.UnsetLoveSceneForActors(Actors)

	If SkyrimOutfitEquipmentSystemNativeFuncs.IsEnabled()
		SkyrimOutfitEquipmentSystemNativeFuncs.SetOutfitsUsingLocation(Game.GetPlayer().GetCurrentLocation(), Weather.GetCurrentWeather())
		SkyrimOutfitEquipmentSystemNativeFuncs.RefreshArmorForAllConfiguredActors()
	Endif
	
	Debug.Notification("Ending OStim thread, stopping SkyOutEquSysOstim.")
EndEvent