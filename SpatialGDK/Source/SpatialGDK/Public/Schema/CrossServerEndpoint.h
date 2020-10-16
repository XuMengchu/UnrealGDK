// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Schema/Component.h"
#include "SpatialConstants.h"
#include "Utils/RPCRingBuffer.h"

#include <WorkerSDK/improbable/c_schema.h>
#include <WorkerSDK/improbable/c_worker.h>

namespace SpatialGDK
{
struct CrossServerEndpoint : Component
{
	CrossServerEndpoint(const Worker_ComponentData& Data, ERPCType Type);

	void ApplyComponentUpdate(const Worker_ComponentUpdate& Update) override;

	RPCRingBuffer ReliableRPCBuffer;

private:
	void ReadFromSchema(Schema_Object* SchemaObject);
};

struct ACKItem
{
	void ReadFromSchema(Schema_Object* SchemaObject);
	void WriteToSchema(Schema_Object* SchemaObject);

	Worker_EntityId Sender = SpatialConstants::INVALID_ENTITY_ID;
	uint64 RPCId = 0;
	uint64 SenderRevision = 0;
};

struct CrossServerEndpointACK : Component
{
	CrossServerEndpointACK(const Worker_ComponentData& Data, ERPCType Type);

	void ApplyComponentUpdate(const Worker_ComponentUpdate& Update) override;

	ERPCType Type;
	uint64 RPCAck = 0;
	TArray<TOptional<ACKItem>> ACKArray;

private:
	void ReadFromSchema(Schema_Object* SchemaObject);
};

struct CrossServerEndpointSender : CrossServerEndpoint
{
	static const Worker_ComponentId ComponentId = SpatialConstants::CROSSSERVER_SENDER_ENDPOINT_COMPONENT_ID;
	CrossServerEndpointSender(const Worker_ComponentData& Data)
		: CrossServerEndpoint(Data, ERPCType::CrossServerSender)
	{
	}
};

struct CrossServerEndpointReceiver : CrossServerEndpoint
{
	static const Worker_ComponentId ComponentId = SpatialConstants::CROSSSERVER_RECEIVER_ENDPOINT_COMPONENT_ID;

	CrossServerEndpointReceiver(const Worker_ComponentData& Data)
		: CrossServerEndpoint(Data, ERPCType::CrossServerReceiver)
	{
	}
};

struct CrossServerEndpointSenderACK : CrossServerEndpointACK
{
	static const Worker_ComponentId ComponentId = SpatialConstants::CROSSSERVER_SENDER_ACK_ENDPOINT_COMPONENT_ID;
	CrossServerEndpointSenderACK(const Worker_ComponentData& Data)
		: CrossServerEndpointACK(Data, ERPCType::CrossServerSender)
	{
	}
};

struct CrossServerEndpointReceiverACK : CrossServerEndpointACK
{
	static const Worker_ComponentId ComponentId = SpatialConstants::CROSSSERVER_RECEIVER_ACK_ENDPOINT_COMPONENT_ID;
	CrossServerEndpointReceiverACK(const Worker_ComponentData& Data)
		: CrossServerEndpointACK(Data, ERPCType::CrossServerReceiver)
	{
	}
};

} // namespace SpatialGDK
