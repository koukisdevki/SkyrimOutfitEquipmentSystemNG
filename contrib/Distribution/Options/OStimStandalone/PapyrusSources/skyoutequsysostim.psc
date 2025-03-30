Scriptname SkyOutEquSysOstim extends Quest  

OsexIntegrationMain ostim 

Event OnInit()
	ostim = OUtils.GetOStim()

	RegisterForModEvent("ostim_thread_start", "OStimThreadStart")
	RegisterForModEvent("ostim_thread_end", "OStimThreadEnd")
EndEvent

Event OStimThreadStart(string EventName, string StrArg, float ThreadID, Form Sender)
	int curentThreadID = ThreadID as Int
	Actor[] ActorArray = OThread.GetActors(curentThreadID)
	SkyrimOutfitEquipmentSystemNativeFuncs.SetLoveSceneForActors(ActorArray)
	
	Debug.Notification("Starting OStim thread, exec SkyOutEquSysOstim.")
EndEvent

Event OStimThreadEnd(string EventName, string Json, float ThreadID, Form Sender)
	; the following code only works with API version 7.3.1 or higher
	Actor[] Actors = OJSON.GetActors(Json)
	SkyrimOutfitEquipmentSystemNativeFuncs.UnsetLoveSceneForActors(Actors)
	Debug.Notification("Ending OStim thread, stopping SkyOutEquSysOstim.")
EndEvent