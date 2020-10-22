// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "Interop/Connection/SpatialEventTracerUserInterface.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialPackageMapClient.h"
#include "Interop/Connection/SpatialEventTracer.h"
#include "Interop/Connection/SpatialWorkerConnection.h"
#include "Interop/SpatialClassInfoManager.h"

DEFINE_LOG_CATEGORY(LogSpatialEventTracerUserInterface);

FUserSpanId USpatialEventTracerUserInterface::CreateSpanId(UObject* WorldContextObject)
{
	const SpatialGDK::SpatialEventTracer* EventTracer = GetEventTracer(WorldContextObject);
	if (EventTracer == nullptr || !EventTracer->IsEnabled())
	{
		return {};
	}

	return SpatialGDK::SpatialEventTracer::SpanIdToUserSpanId(EventTracer->CreateSpan().GetValue());
}

FUserSpanId USpatialEventTracerUserInterface::CreateSpanIdWithCauses(UObject* WorldContextObject, const TArray<FUserSpanId>& Causes)
{
	const SpatialGDK::SpatialEventTracer* EventTracer = GetEventTracer(WorldContextObject);
	if (EventTracer == nullptr || !EventTracer->IsEnabled())
	{
		return {};
	}

	TArray<Trace_SpanId> SpanIds;
	for (const FUserSpanId& UserSpanIdCause : Causes)
	{
		if (!UserSpanIdCause.IsValid())
		{
			continue;
		}

		TOptional<Trace_SpanId> CauseSpanId = SpatialGDK::SpatialEventTracer::UserSpanIdToSpanId(UserSpanIdCause);
		if (CauseSpanId.IsSet())
		{
			SpanIds.Add(CauseSpanId.GetValue());
		}
	}

	return SpatialGDK::SpatialEventTracer::SpanIdToUserSpanId(EventTracer->CreateSpan(SpanIds.GetData(), SpanIds.Num()).GetValue());
}

void USpatialEventTracerUserInterface::TraceEvent(UObject* WorldContextObject, const FUserSpanId& UserSpanId,
												  FSpatialTraceEvent SpatialTraceEvent)
{
	SpatialGDK::SpatialEventTracer* EventTracer = GetEventTracer(WorldContextObject);
	if (EventTracer == nullptr)
	{
		return;
	}

	if (!UserSpanId.IsValid())
	{
		return;
	}

	TOptional<Trace_SpanId> SpanId = SpatialGDK::SpatialEventTracer::UserSpanIdToSpanId(UserSpanId);
	if (!SpanId.IsSet())
	{
		return;
	}

	EventTracer->TraceEvent(SpatialTraceEvent, SpanId.GetValue());
}

void USpatialEventTracerUserInterface::TraceRPC(UObject* WorldContextObject, FEventTracerRPCDelegate Delegate,
													   const FUserSpanId& UserSpanId)
{
	SpatialGDK::SpatialEventTracer* EventTracer = GetEventTracer(WorldContextObject);
	if (EventTracer == nullptr || !EventTracer->IsEnabled())
	{
		return;
	}

	if (!UserSpanId.IsValid())
	{
		return;
	}

	TOptional<Trace_SpanId> SpanId = SpatialGDK::SpatialEventTracer::UserSpanIdToSpanId(UserSpanId);
	if (!SpanId.IsSet())
	{
		return;
	}

	EventTracer->SpanIdStack.Stack(SpanId.GetValue());
	Delegate.Execute();
	EventTracer->SpanIdStack.PopLayer();
}

bool USpatialEventTracerUserInterface::GetActiveSpanId(UObject* WorldContextObject, FUserSpanId& OutUserSpanId)
{
	const SpatialGDK::SpatialEventTracer* EventTracer = GetEventTracer(WorldContextObject);
	if (EventTracer == nullptr || !EventTracer->IsEnabled())
	{
		return false;
	}

	if (!EventTracer->SpanIdStack.HasLayer())
	{
		return false;
	}

	TOptional<Trace_SpanId> SpanId = EventTracer->SpanIdStack.GetTopSpanId();
	if (!SpanId.IsSet())
	{
		return false;
	}

	OutUserSpanId = SpatialGDK::SpatialEventTracer::SpanIdToUserSpanId(SpanId.GetValue());
	return true;
}

void USpatialEventTracerUserInterface::AddLatentActorSpanId(UObject* WorldContextObject, const AActor& Actor, const FUserSpanId& UserSpanId)
{
	SpatialGDK::SpatialEventTracer* EventTracer = GetEventTracer(WorldContextObject);
	if (EventTracer == nullptr || !EventTracer->IsEnabled())
	{
		return;
	}

	USpatialNetDriver* NetDriver = GetSpatialNetDriver(WorldContextObject);
	if (NetDriver == nullptr)
	{
		return;
	}

	TOptional<Trace_SpanId> SpanId = SpatialGDK::SpatialEventTracer::UserSpanIdToSpanId(UserSpanId);
	if (!SpanId.IsSet())
	{
		return;
	}

	const Worker_EntityId EntityId = NetDriver->PackageMap->GetEntityIdFromObject(&Actor);
	const Worker_ComponentId ComponentId = NetDriver->ClassInfoManager->GetComponentIdForClass(*Actor.GetClass());
	EventTracer->AddLatentPropertyUpdateSpanIds({ EntityId, ComponentId }, SpanId.GetValue());
}

void USpatialEventTracerUserInterface::AddLatentComponentSpanId(UObject* WorldContextObject, const UActorComponent& Component,
																const FUserSpanId& UserSpanId)
{
	SpatialGDK::SpatialEventTracer* EventTracer = GetEventTracer(WorldContextObject);
	if (EventTracer == nullptr || !EventTracer->IsEnabled())
	{
		return;
	}

	USpatialNetDriver* NetDriver = GetSpatialNetDriver(WorldContextObject);
	if (NetDriver == nullptr)
	{
		return;
	}

	AActor* Owner = Component.GetOwner();
	if (Owner == nullptr)
	{
		UE_LOG(LogSpatialEventTracerUserInterface, Error,
			   TEXT("USpatialEventTracerUserInterface::AddLatentComponentSpanId - Component has now owner"));
		return;
	}

	TOptional<Trace_SpanId> SpanId = SpatialGDK::SpatialEventTracer::UserSpanIdToSpanId(UserSpanId);
	if (!SpanId.IsSet())
	{
		return;
	}

	const Worker_EntityId EntityId = NetDriver->PackageMap->GetEntityIdFromObject(Owner);
	const Worker_ComponentId ComponentId =
		NetDriver->ClassInfoManager->GetComponentIdForSpecificSubObject(*Owner->GetClass(), *Component.GetClass());
	EventTracer->AddLatentPropertyUpdateSpanIds({ EntityId, ComponentId }, SpanId.GetValue());
}

void USpatialEventTracerUserInterface::TraceProperty(UObject* WorldContextObject, UObject* Object, const FUserSpanId& UserSpanId)
{
	if (AActor* Actor = Cast<AActor>(Object))
	{
		AddLatentActorSpanId(WorldContextObject, *Actor, UserSpanId);
	}
	else if (UActorComponent* Component = Cast<UActorComponent>(Object))
	{
		AddLatentComponentSpanId(WorldContextObject, *Component, UserSpanId);
	}
	else
	{
		UE_LOG(LogSpatialEventTracerUserInterface, Warning,
			   TEXT("USpatialEventTracerUserInterface::AddLatentSpanId - Could not add latent SpanId for %s"), *Object->GetName());
	}
}

SpatialGDK::SpatialEventTracer* USpatialEventTracerUserInterface::GetEventTracer(UObject* WorldContextObject)
{
	const USpatialNetDriver* NetDriver = GetSpatialNetDriver(WorldContextObject);
	if (NetDriver == nullptr || NetDriver->Connection == nullptr)
	{
		UE_LOG(LogSpatialEventTracerUserInterface, Error,
			   TEXT("USpatialEventTracerUserInterface::GetEventTracer - NetDriver or Connection is null"));
		return nullptr;
	}

	return NetDriver->Connection->GetEventTracer();
}

USpatialNetDriver* USpatialEventTracerUserInterface::GetSpatialNetDriver(UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World == nullptr)
	{
		UE_LOG(LogSpatialEventTracerUserInterface, Error, TEXT("USpatialEventTracerUserInterface::GetSpatialNetDriver - World is null"));
		World = GWorld;
	}

	USpatialNetDriver* NetDriver = Cast<USpatialNetDriver>(World->GetNetDriver());
	return NetDriver;
}