Scriptname SkyOutSysAutoSwitchTrigger extends ReferenceAlias

Weather Property LastWeather Auto
Bool Property IsSwimming = false Auto
Location Property LastLocation Auto

; Timer interval in seconds (real time)
Float Property UpdateInterval = 3.0 Auto  

Event OnInit()
    InitializeProperties()
EndEvent

Event OnPlayerLoadGame()
    InitializeProperties()
EndEvent

Function InitializeProperties()
    LastWeather = Weather.GetCurrentWeather()
    LastLocation = Game.GetPlayer().GetCurrentLocation()
    IsSwimming = Game.GetPlayer().IsSwimming()
    RegisterForSingleUpdate(UpdateInterval)
EndFunction

; Use real-time update for periodic checks
Event OnUpdate()
    If GetState() == "Listening"
        CheckForChanges()
    EndIf
    RegisterForSingleUpdate(UpdateInterval)
EndEvent

; Main function to check for changes
Function CheckForChanges()
    If GetState() != "Listening"
        Return
    EndIf
    
    ; Check for location changes
    Location currentLocation = Game.GetPlayer().GetCurrentLocation()
    If currentLocation != LastLocation
        If currentLocation && currentLocation.GetName() != ""
            UpdateOutfits("Location changed to " + currentLocation.GetName())
        Else
            UpdateOutfits("Location changed")
        EndIf
        LastLocation = currentLocation
        Return ; Return to prevent multiple updates in the same check
    EndIf
    
    ; Check for weather changes
    Weather currentWeather = Weather.GetCurrentWeather()
    If currentWeather != LastWeather && currentWeather
        UpdateOutfits("Weather changed to " + GetWeatherName(currentWeather))
        LastWeather = currentWeather
        Return ; Return to prevent multiple updates in the same check
    EndIf
    
    ; Check for swimming status changes
    Bool currentSwimming = Game.GetPlayer().IsSwimming()
    If currentSwimming != IsSwimming
        String outMessage = ""
        If currentSwimming
            outMessage  = "Started swimming"
        Else
            outMessage  = "Stopped swimming"
        EndIf
        UpdateOutfits(outMessage)
        IsSwimming = currentSwimming
    EndIf
EndFunction

; Central update function to avoid code duplication
Function UpdateOutfits(String notificationMessage)
    GoToState("Waiting")
    Utility.Wait(1.0)
    Debug.Notification("Outfit System: " + notificationMessage)
    SkyrimOutfitSystemNativeFuncs.SetOutfitsUsingLocation(Game.GetPlayer().GetCurrentLocation(), Weather.GetCurrentWeather())
    SkyrimOutfitSystemNativeFuncs.RefreshArmorForAllConfiguredActors()
    GoToState("Listening")
EndFunction

; Helper function to get a readable weather name
String Function GetWeatherName(Weather akWeather)
    If !akWeather
        Return "Unknown"
    EndIf
    
    ; Try to get weather classification
    If akWeather.GetClassification() == 0
        Return "Pleasant"
    ElseIf akWeather.GetClassification() == 1
        Return "Cloudy"
    ElseIf akWeather.GetClassification() == 2
        Return "Rainy"
    ElseIf akWeather.GetClassification() == 3
        Return "Snow"
    Else
        Return "Weather " + akWeather
    EndIf
EndFunction

Auto State Listening
    ; Using base events
EndState

State Waiting
    ; Empty state - now everything is handled by the timer
EndState