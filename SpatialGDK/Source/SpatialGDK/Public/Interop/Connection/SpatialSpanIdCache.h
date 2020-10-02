// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "Interop/EntityRPCType.h"
#include "Misc/DateTime.h"
#include "SpatialView/EntityComponentId.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSpatialSpanIdStore, Log, All);

namespace SpatialGDK
{
class SpatialSpanIdCache
{
public:
	struct FieldSpanIdUpdate
	{
		uint32 FieldId;
		worker::c::Trace_SpanId NewSpanId;
		worker::c::Trace_SpanId OldSpanId;
	};

	void ComponentAdd(const Worker_Op& Op);
	bool ComponentRemove(const Worker_Op& Op);

	// Returns a list of the field ids that already existed in the store
	TArray<FieldSpanIdUpdate> ComponentUpdate(const Worker_Op& Op);

	void AddSpanId(const EntityComponentId& Id, const uint32 FieldId, worker::c::Trace_SpanId SpanId);
	bool DropSpanId(const EntityComponentId& Id, const uint32 FieldId);
	bool DropSpanIds(const EntityComponentId& Id);

	bool GetSpanId(const EntityComponentId& Id, const uint32 FieldId, worker::c::Trace_SpanId& OutSpanId, bool bRemove = true);
	bool GetMostRecentSpanId(const EntityComponentId& Id, worker::c::Trace_SpanId& OutSpanId, bool bRemove = true);

private:
	struct EntityComponentFieldId
	{
		EntityComponentId EntityComponentId;
		uint32 FieldId;
	};

	struct EntityComponentFieldIdSpanIdUpdate
	{
		worker::c::Trace_SpanId SpanId = worker::c::Trace_SpanId();
		FDateTime UpdateTime;
	};

	using FieldIdMap = TMap<uint32, EntityComponentFieldIdSpanIdUpdate>;
	TMap<EntityComponentId, FieldIdMap> EntityComponentFieldSpanIds;

	bool DropSpanIdInternal(FieldIdMap* SpanIdMap, const EntityComponentId& Id, const uint32 FieldId);
};

} // namespace SpatialGDK