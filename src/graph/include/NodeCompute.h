// Created by Tyler on 4/30/2026.
#pragma once

#include "GraphTypes.h"

START_NAMESPACE(ATGraph)

consteval NodeTypeID nodeTypeHash(const std::string_view str)
{
    constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
    uint64_t hash = FNV_OFFSET_BASIS;
    for (const char c : str)
    {
        constexpr uint64_t FNV_PRIME = 0x00000100000001B3ULL;
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= FNV_PRIME;
    }
    return hash;
}

// Every unique type of node must have this implemented.
// NodeTypeID -> INodeCompute. Only one instance of that node type is instanced.
// When there are multiple nodes instances of the same type, then same INodeCompute is used.
class INodeCompute
{
public:
    /// The graph passes in `this` nodes `record` and the graphs
    /// data store where the node will read and write its input to...
    virtual void compute(const NodeRecord& nodeRecord, DataStore& dStore) = 0;
    /// NOTE: the order you return these is important. When a node compute is called
    /// A node can identify specific attributes with an index.
    /// Example: if you return {float, float, quat, transform}
    /// A node can load the quat first via: nodeRecord.inputAttrIDs[2].
    /// The graph always creates the attribute records in the order returned.
    NDESC virtual constexpr std::span<const AttributeDescriptor> inputAttrSchema() const = 0;
    /// read inputAttrSchema summary. Same goes for outputAttrSchema.
    NDESC virtual constexpr std::span<const AttributeDescriptor> outputAttrSchema() const = 0;
    NDESC virtual constexpr NodeTypeID nodeTypeID() const = 0;
    virtual ~INodeCompute() = default;
};

END_NAMESPACE
