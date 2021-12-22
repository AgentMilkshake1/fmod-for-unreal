// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2021.

#pragma once

#include "Containers/Map.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Sound/SoundAttenuation.h"
#include "AudioDevice.h"
#include "FMODStudioModule.h"
#include "FMODUtils.h"
#include "FMODAudioComponent.generated.h"

// Event property
UENUM()
namespace EFMODEventProperty
{
enum Type
{
    /* Priority to set on low-level channels created by this event instance (-1 to 256). */
    ChannelPriority,
    /** Schedule delay to synchronized playback for multiple tracks in DSP clocks, or -1 for default. */
    ScheduleDelay,
    /** Schedule look-ahead on the timeline in DSP clocks, or -1 for default. */
    ScheduleLookahead,
    /** Override the event's 3D minimum distance, or -1 for default. */
    MinimumDistance,
    /** Override the event's 3D maximum distance, or -1 for default. */
    MaximumDistance,
    /** Number of options */
    Count
};
}


/** Used to store callback info from FMOD thread to our event */
struct FTimelineMarkerProperties
{
    FString Name;
    int32 Position;
};

/** Used to store callback info from FMOD thread to our event */
struct FTimelineBeatProperties
{
    int32 Bar;
    int32 Beat;
    int32 Position;
    float Tempo;
    int32 TimeSignatureUpper;
    int32 TimeSignatureLower;
};

USTRUCT(BlueprintType)
struct FFMODAttenuationDetails
{
    GENERATED_USTRUCT_BODY()

    /** Should we use Attenuation set in Studio or be able to modify in Editor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Attenuation")
    uint32 bOverrideAttenuation : 1;

    /** Override the event's 3D minimum distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Attenuation",
        meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bOverrideAttenuation"))
    float MinimumDistance;

    /** Override the event's 3D maximum distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Attenuation",
        meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bOverrideAttenuation"))
    float MaximumDistance;

    FFMODAttenuationDetails()
        : bOverrideAttenuation(false)
        , MinimumDistance(1.0f)
        , MaximumDistance(10.0f)
    {}
};

USTRUCT(BlueprintType)
struct FFMODOcclusionDetails
{
    GENERATED_USTRUCT_BODY()

    /** Enable Occlusion Settings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Occlusion")
    bool bEnableOcclusion;

    /* Which trace channel to use for audio occlusion checks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Occlusion", meta = (EditCondition = "bEnableOcclusion"))
    TEnumAsByte<enum ECollisionChannel> OcclusionTraceChannel;

    /** Whether or not to enable complex geometry occlusion checks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FMOD|Occlusion", meta=(EditCondition = "bEnableOcclusion"))
    bool bUseComplexCollisionForOcclusion;

    FFMODOcclusionDetails()
        : bEnableOcclusion(false)
        , OcclusionTraceChannel(ECC_Visibility)
        , bUseComplexCollisionForOcclusion(false)
    {}
};

/** called when an event stops, either because it played to completion or because a Stop() call turned it off early */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEventStopped);
/** called when we reach a named marker on the timeline */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimelineMarker, FString, Name, int32, Position);
/** called when we reach a beat on the timeline */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(
    FOnTimelineBeat, int32, Bar, int32, Beat, int32, Position, float, Tempo, int32, TimeSignatureUpper, int32, TimeSignatureLower);

namespace FMOD
{
class DSP;
class Sound;

namespace Studio
{
class EventDescription;
class EventInstance;
}
}

struct FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES;
struct FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES;

/**
 * Plays FMOD Studio events.
 */
UCLASS(Blueprintable, ClassGroup = (Audio, Common), hidecategories = (Object, ActorComponent, Physics, Rendering, Mobility, LOD),
    ShowCategories = Trigger, meta = (BlueprintSpawnableComponent))
class FMODSTUDIO_API UFMODAudioComponent : public USceneComponent
{
    GENERATED_UCLASS_BODY()
public:

    /** The event asset to use for this sound. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    TAssetPtr<class UFMODEvent> Event;

    /** Event parameter cache. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SimpleDisplay, Category = FMODAudio)
    TMap<FName, float> ParameterCache;
    bool bDefaultParameterValuesCached;

    /** Sound name used for programmer sound.  Will look up the name in any loaded audio table. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    FString ProgrammerSoundName;

    /** Enable timeline callbacks for this sound, so that OnTimelineMarker and OnTimelineBeat can be used. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    uint32 bEnableTimelineCallbacks : 1;

    /** Stored properties to apply next time we create an instance. */
    float StoredProperties[EFMODEventProperty::Count];

    /** Auto destroy this component on completion. */
    UPROPERTY()
    uint32 bAutoDestroy : 1;

    /** Stop sound when owner is destroyed. */
    UPROPERTY()
    uint32 bStopWhenOwnerDestroyed : 1;

    /** Whether we apply gain and low-pass based on audio zones. */
    uint32 bApplyAmbientVolumes : 1;

    /** Whether we apply gain and low-pass based on occlusion onto a parameter. */
    uint32 bApplyOcclusionParameter:1;

    /** Called when an event stops, either because it played to completion or because a Stop() call turned it off early. */
    UPROPERTY(BlueprintAssignable)
    FOnEventStopped OnEventStopped;

    /** Called when we reach a named marker (if bEnableTimelineCallbacks is true). */
    UPROPERTY(BlueprintAssignable)
    FOnTimelineMarker OnTimelineMarker;

    /** Called when we reach a beat of a tempo (if bEnableTimelineCallbacks is true). */
    UPROPERTY(BlueprintAssignable)
    FOnTimelineBeat OnTimelineBeat;

    /** New Event to be used by the FMODAudioComponent. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetEvent(UFMODEvent *NewEvent);

    /** Start a sound playing on an audio component. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void Play();

    /** Stop an audio component playing its sound cue, issue any delegates if needed. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void Stop();

    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void Release();

    /**  Allow an event to continue past a sustain point. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void KeyOff();

    /** Return true if this component is currently playing an event. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    bool IsPlaying();

    /** Sets the volume level. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetVolume(float volume);

    /** Sets the pitch multiplier. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetPitch(float pitch);

    /** Pause/Unpause an audio component. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetPaused(bool paused);

    /** Set a parameter of the Event. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetParameter(FName Name, float Value);

    /** Will be deprecated in FMOD 2.01, use `GetParameterValue(FName, float, float)` instead.
     * Get parameter value from the Event.
    */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    float GetParameter(FName Name);

    /** Get parameter value from the Event.
     * @param Name - Name of parameter
     * @param UserValue - Parameter value as set from the public API.
     * @param FinalValue - Final combined parameter value.
    */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void GetParameterValue(FName Name, float &UserValue, float &FinalValue);

    /** Set a property of the Event. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetProperty(EFMODEventProperty::Type Property, float Value);

    /** Get a property of the Event. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    float GetProperty(EFMODEventProperty::Type Property);

    /** Get the event length in milliseconds. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    int32 GetLength() const;

    /** Set the timeline position in milliseconds. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetTimelinePosition(int32 Time);

    /** Get the timeline position in milliseconds. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    int32 GetTimelinePosition();

    /** Called when the event has finished stopping. */
    void OnPlaybackCompleted();

    /** Update gain and low-pass based on interior volumes. */
    void UpdateInteriorVolumes();

    /** Set the sound name to use for programmer sound.  Will look up the name in any loaded audio table. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetProgrammerSoundName(FString Value);

    /** Set a programmer sound to use for this audio component.  Lifetime of sound must exceed that of the audio component. */
    void SetProgrammerSound(FMOD::Sound *Sound);

    /** FMOD Custom Attenuation Details. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    struct FFMODAttenuationDetails AttenuationDetails;

    /** FMOD Custom Occlusion Details. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    struct FFMODOcclusionDetails OcclusionDetails;

    /** Update attenuation if we have it set. */
    void UpdateAttenuation();

    /** Apply Volume and LPF into event. */
    void ApplyVolumeLPF();

    /** Cache default event parameter values. */
    void CacheDefaultParameterValues();

public:
    /** Internal play function which can play events in the editor. */
    void PlayInternal(EFMODSystemContext::Type Context);

    /** Actual Studio instance handle. */
    FMOD::Studio::EventInstance *StudioInstance;

    /** Timeline Marker callback. */
    void EventCallbackAddMarker(struct FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES *props);

    /** Timeline Beat callback. */
    void EventCallbackAddBeat(struct FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES *props);

    /** Programmer Sound Create callback. */
    void EventCallbackCreateProgrammerSound(struct FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props);

    /** Programmer Sound Destroy callback. */
    void EventCallbackDestroyProgrammerSound(struct FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props);

// Begin UObject interface.
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent &e) override;
#endif // WITH_EDITOR
    virtual void PostLoad() override;
    virtual FString GetDetailedInfoInternal() const override;
    // End UObject interface.
    // Begin USceneComponent Interface
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;
    virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;
    // End USceneComponent Interface

private:
// Begin ActorComponent interface.
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
// End ActorComponent interface.

#if WITH_EDITORONLY_DATA
    void UpdateSpriteTexture();
#endif

    /** Release any cached parameters then the Studio Instance. */
    void ReleaseEventCache();

    /** Release the Studio Instance. */
    void ReleaseEventInstance();

    /** Return a cached reference to the current IFMODStudioModule.*/
    IFMODStudioModule& GetStudioModule()
    {
        if (Module == nullptr)
        {
            Module = &IFMODStudioModule::Get();
        }
        return *Module;
    }
    IFMODStudioModule* Module;

    // Settings for ambient volume effects.
    double InteriorLastUpdateTime;
    float SourceInteriorVolume;
    float SourceInteriorLPF;
    float CurrentInteriorVolume;
    float CurrentInteriorLPF;
    float AmbientVolume;
    float AmbientLPF;
    float LastVolume;
    float LastLPF;
    bool wasOccluded;
    FMOD_STUDIO_PARAMETER_ID OcclusionID;
    FMOD_STUDIO_PARAMETER_ID AmbientVolumeID;
    FMOD_STUDIO_PARAMETER_ID AmbientLPFID;

    // Tempo and marker callbacks.
    FCriticalSection CallbackLock;
    TArray<FTimelineMarkerProperties> CallbackMarkerQueue;
    TArray<FTimelineBeatProperties> CallbackBeatQueue;

    // Direct assignment of programmer sound from other C++ code.
    FMOD::Sound *ProgrammerSound;
    bool NeedDestroyProgrammerSoundCallback;
    int32 EventLength;
};
