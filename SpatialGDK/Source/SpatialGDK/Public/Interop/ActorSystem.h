// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once
#include "Schema/NetOwningClientWorker.h"
#include "Schema/SpawnData.h"
#include "Schema/UnrealMetadata.h"

DECLARE_LOG_CATEGORY_EXTERN(LogActorSystem, Log, All);

struct FPendingSubobjectAttachment;
class USpatialNetConnection;
class FSpatialObjectRepState;
class FRepLayout;
struct FClassInfo;
class USpatialNetDriver;

class SpatialActorChannel;
class USpatialNetDriver;

namespace SpatialGDK
{
class FSubView;

struct ActorData
{
	SpawnData Spawn;
	UnrealMetadata Metadata;
	NetOwningClientWorker OwningClientWorker;
};

class ActorSystem
{
public:
	ActorSystem(const FSubView& InSubView, USpatialNetDriver* InNetDriver, FTimerManager* InTimerManager);

	void Advance();

	void CleanupRepStateMap(FSpatialObjectRepState& RepState);

	TMap<TPair<Worker_EntityId_Key, Worker_ComponentId>, TSharedRef<FPendingSubobjectAttachment>> PendingEntitySubobjectDelegations;

private:
	// Helper struct to manage FSpatialObjectRepState update cycle.
	// TODO: move into own class.
	struct RepStateUpdateHelper;

	struct DeferredRetire
	{
		Worker_EntityId EntityId;
		Worker_ComponentId ActorClassId;
		bool bIsNetStartupActor;
		bool bNeedsTearOff;
	};
	TArray<DeferredRetire> EntitiesToRetireOnAuthorityGain;

	// Map from references to replicated objects to properties using these references.
	// Useful to manage entities going in and out of interest, in order to recover references to actors.
	FObjectToRepStateMap ObjectRefToRepStateMap;

	void PopulateDataStore(Worker_EntityId EntityId);
	void ApplyComponentAdd(Worker_EntityId EntityId, Worker_ComponentId ComponentId, Schema_ComponentData* Data);
	void ApplyComponentUpdate(Worker_EntityId EntityId, Worker_ComponentId ComponentId, Schema_ComponentUpdate* Update);

	void AuthorityLost(Worker_EntityId EntityId, Worker_ComponentId ComponentId);
	void AuthorityGained(Worker_EntityId EntityId, Worker_ComponentId ComponentId);
	void HandleActorAuthority(Worker_EntityId EntityId, Worker_ComponentId ComponentId, Worker_Authority Authority);

	void ComponentAdded(Worker_EntityId EntityId, Worker_ComponentId ComponentId, Schema_ComponentData* Data);
	void ComponentUpdated(Worker_EntityId EntityId, Worker_ComponentId ComponentId, Schema_ComponentUpdate* Update);
	void ComponentRemoved(Worker_EntityId EntityId, Worker_ComponentId ComponentId) const;

	void EntityAdded(Worker_EntityId EntityId);
	void EntityRemoved(Worker_EntityId EntityId);

	// Auth
	bool HasEntityBeenRequestedForDelete(Worker_EntityId EntityId) const;
	void HandleEntityDeletedAuthority(Worker_EntityId EntityId) const;
	void HandleDeferredEntityDeletion(const DeferredRetire& Retire) const;
	void HandlePlayerLifecycleAuthority(const Worker_EntityId EntityId, const Worker_ComponentId ComponentId,
										const Worker_Authority Authority, APlayerController* PlayerController);
	void UpdateShadowData(Worker_EntityId EntityId) const;

	// Component add
	void HandleDormantComponentAdded(Worker_EntityId EntityId) const;
	void HandleIndividualAddComponent(Worker_EntityId EntityId, Worker_ComponentId ComponentId, Schema_ComponentData* Data);
	void AttachDynamicSubobject(AActor* Actor, Worker_EntityId EntityId, const FClassInfo& Info);
	void ApplyComponentData(USpatialActorChannel& Channel, UObject& TargetObject, const Worker_ComponentId ComponentId,
							Schema_ComponentData* Data);
	bool IsDynamicSubObject(AActor* Actor, uint32 SubObjectOffset);
	void ResolvePendingOperations(UObject* Object, const FUnrealObjectRef& ObjectRef);
	void ResolveIncomingOperations(UObject* Object, const FUnrealObjectRef& ObjectRef);
	void ResolveObjectReferences(FRepLayout& RepLayout, UObject* ReplicatedObject, FSpatialObjectRepState& RepState,
								 FObjectReferencesMap& ObjectReferencesMap, uint8* RESTRICT StoredData, uint8* RESTRICT Data,
								 int32 MaxAbsOffset, TArray<GDK_PROPERTY(Property) *>& RepNotifies, bool& bOutSomeObjectsWereMapped);

	// Component update
	void OnHeartbeatComponentUpdate(const Worker_EntityId EntityId, Schema_ComponentUpdate* Update);
	USpatialActorChannel* GetOrRecreateChannelForDomantActor(AActor* Actor, Worker_EntityId EntityID) const;
	void ApplyComponentUpdate(Worker_ComponentId ComponentId, Schema_ComponentUpdate* ComponentUpdate, UObject& TargetObject,
							  USpatialActorChannel& Channel, bool bIsHandover);

	// Entity add
	void ReceiveActor(Worker_EntityId EntityId);
	bool IsReceivedEntityTornOff(Worker_EntityId EntityId) const;
	AActor* TryGetOrCreateActor(ActorData& ActorComponents);
	AActor* CreateActor(ActorData& ActorComponents);
	void ApplyComponentDataOnActorCreation(const Worker_EntityId EntityId, const Worker_ComponentId ComponentId, Schema_ComponentData* Data,
										   USpatialActorChannel& Channel, TArray<ObjectPtrRefPair>& OutObjectsToResolve);

	// Entity remove
	void RemoveActor(Worker_EntityId EntityId);
	void DestroyActor(AActor* Actor, Worker_EntityId EntityId);
	void MoveMappedObjectToUnmapped(const FUnrealObjectRef& Ref);
	static FString GetObjectNameFromRepState(const FSpatialObjectRepState& RepState);
	void CloseClientConnection(USpatialNetConnection* ClientConnection, Worker_EntityId PlayerControllerEntityId);

	// Helper
	bool EntityHasComponent(Worker_EntityId EntityId, Worker_ComponentId ComponentId) const;

	const FSubView* SubView;
	USpatialNetDriver* NetDriver;
	FTimerManager* TimerManager;

	// This will map PlayerController entities to the corresponding SpatialNetConnection
	// for PlayerControllers that this server has authority over. This is used for player
	// lifecycle logic (Heartbeat component updates, disconnection logic).
	TMap<Worker_EntityId_Key, TWeakObjectPtr<USpatialNetConnection>> AuthorityPlayerControllerConnectionMap;

	TMap<Worker_EntityId_Key, FString> WorkerConnectionEntities;

	TSet<TPair<Worker_EntityId_Key, Worker_ComponentId>> PendingDynamicSubobjectComponents;

	// Deserialized state store for Actor relevant components.
	TMap<Worker_EntityId_Key, ActorData> ActorDataStore;
};

} // namespace SpatialGDK