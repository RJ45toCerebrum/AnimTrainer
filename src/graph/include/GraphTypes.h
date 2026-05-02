// Created by Tyler on 4/29/2026.
#pragma once

#include "common.h"

#include <glm/vec3.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>
#include <string>
#include <memory_resource>
#include <format>

START_NAMESPACE(ATGraph)

using NodeID = uint32_t;
using NodeTypeID = uint32_t;
using AttrID = uint32_t;
///TODO: Later need to use the graph allocator for mem pools
using GraphDataAllocator = std::pmr::unsynchronized_pool_resource;

// TODO: you could accidentally compare with wrong ID. I should make this type safe.
constexpr NodeID kInvalidNodeID = 0;
constexpr NodeTypeID kInvalidNodeTypeID = 0;
constexpr AttrID kInvalidAttr = std::numeric_limits<AttrID>::max();

enum class AttributeDirection : uint8_t
{
    Input,
    Output
};

enum class AttributeDataType : uint8_t
{
    Float,
    Vec3,
    Transform
};

consteval std::string_view attrDataTypeStr(const AttributeDataType type)
{
    static constexpr std::string_view kInvalidDataTypeStr = "InvalidDataType";
    static constexpr std::string_view kFloatAttrStr = "float";
    static constexpr std::string_view kVec3AttrStr = "vec3";
    static constexpr std::string_view kTransformStr = "transform";
    // TODO: finish
    switch (type)
    {
        case AttributeDataType::Float:
            return kFloatAttrStr;
        case AttributeDataType::Vec3:
            return kVec3AttrStr;
        case AttributeDataType::Transform:
            return kTransformStr;
    }
    return kInvalidDataTypeStr;
}

/// TODO: finish
template<typename T>
consteval AttributeDataType enumFromAttrType()
{
    if constexpr (std::is_same_v<T, float>)
        return AttributeDataType::Float;
    else if constexpr (std::is_same_v<T, glm::vec3>)
        return AttributeDataType::Vec3;

    throw std::exception("Type T not implemented yet");
}

inline int sizeOfFromAttrType(const AttributeDataType attrType)
{
    if (attrType == AttributeDataType::Float)
        return sizeof(float);
    if (attrType == AttributeDataType::Vec3)
        return sizeof(glm::vec3);

    throw std::exception("Type T not implemented yet");
}

/// TODO: finish
template<typename T>
concept AttributeTypeConcept =
    std::same_as<T, float> || std::same_as<T, glm::vec3>;

/// Attributes are a fundamental component to the graph.
/// Every node has input attributes and output attributes.
/// every attribute record is placed in the graphs flat list of attribute records.
/// At a fundamental level, these records store the topological structure of the graph.
struct AttributeRecord final
{
    // The Attributes ID itself is always the same as its index in graphs attributeRecords array
    // This attribute record does not need to store its own ID because of this.
    NodeID owner = kInvalidNodeID;
    AttributeDirection direction = AttributeDirection::Input;
    AttributeDataType type = AttributeDataType::Float;
    // upstream only valid for input attrs ; downstream only valid for output attrs
    // I will save space later, but for first iteration making one invalid is fine
    AttrID upstream = kInvalidAttr;
    std::vector<AttrID> downstream;

    NDESC bool isTombstone() const
    {
        return owner == kInvalidNodeID;
    }
    NDESC bool isInputAttr() const
    {
        assert(not isTombstone());
        return direction == AttributeDirection::Input;
    }
    NDESC bool isOutputAttr() const
    {
        assert(not isTombstone());
        return direction == AttributeDirection::Output;
    }
    NDESC bool hasSources() const
    {
        assert(not isTombstone());
        if (direction == AttributeDirection::Input)
            return upstream != kInvalidAttr;
        if (downstream.empty())
            return false;
        return std::ranges::any_of(downstream, [](const AttrID attrID) -> bool
        {
            return attrID != kInvalidAttr;
        });
    }
    /// only returns true IF input attr and is plugged
    NDESC bool hasInputSources() const
    {
        assert(not isTombstone());
        if (not isInputAttr())
            return false;
        // an input attribute should never downstream plugs
        assert(downstream.empty());
        return upstream != kInvalidAttr;
    }
    /// only returns true IF output attr and is plugged into input attr downstream.
    NDESC bool hasOutputSources() const
    {
        assert(not isTombstone());
        if (not isOutputAttr())
            return false;
        // an output attribute should never have upstream plugged
        assert(upstream == kInvalidAttr);
        if (downstream.empty())
            return false;
        return std::ranges::any_of(downstream, [](const AttrID attrID) -> bool
        {
            return attrID != kInvalidAttr;
        });
    }

    void invalidate()
    {
        owner = kInvalidNodeID;
        upstream = kInvalidAttr;
        downstream.clear();
    }
    // TODO: toString
};


/// for every attribute there is a DataSlot entry in the graph. meaning,
/// the length of the attribute record list and the length of graphs data slot list are equal
/// at all times. When a nodes output attr gets connected to input attr, this results in
/// that data slot that was previously allocated to become dead memory.
/// Later we will update to reclaim, but for the time being I leave this and if it becomes
/// disconnected, then it becomes useful again.
struct DataSlot final
{
    std::vector<std::byte> bytes;
    // this version flag is key to efficiency of the graph.
    // If there is a mismatch between this value and NodeRecord[attrID].lastSeenVersions
    // this means the graph needs recompute.
    uint64_t version = 0;

    template<AttributeTypeConcept T>
    std::span<const T> readAsSpan() const
    {
        assert(reinterpret_cast<uintptr_t>(bytes.data()) % alignof(T) == 0
        && "DataSlot buffer insufficiently aligned for type T");
        const T* rawData = reinterpret_cast<const T*>(bytes.data());
        return {rawData, bytes.size() / sizeof(T)};
    }

    NDESC std::span<const std::byte> readAsBytes() const
    {
        return {bytes.data(), bytes.size()};
    }

    template<AttributeTypeConcept T>
    void writeAsSpan(const std::span<const T> values)
    {
        const auto* begin = reinterpret_cast<const std::byte*>(values.data());
        bytes.assign(begin, begin + values.size_bytes());
        ++version;
    }

    void writeRawBytes(const std::span<const std::byte> data)
    {
        bytes.assign(data.begin(), data.end());
        version++;
    }

    template<AttributeTypeConcept T>
    static std::span<const std::byte> convert(std::span<const T> data)
    {
        const auto* begin = reinterpret_cast<const std::byte*>(data.data());
        return {begin, data.size_bytes()};
    }

    template<AttributeTypeConcept T>
    static std::span<const T> convert(const std::span<const std::byte> data)
    {
        const T* rawData = reinterpret_cast<const T*>(data.data());
        assert(reinterpret_cast<uintptr_t>(rawData) % alignof(T) == 0
            && "data buffer insufficiently aligned for type T");
        return {rawData, data.size_bytes() / sizeof(T)};
    }
};


/// Every created node has a NodeRecord
/// typeID => INodeCompute
struct NodeRecord final
{
    // the nodes ID is its index into graphs node records array. No need to store here.
    NodeTypeID typeID = kInvalidNodeTypeID;
    std::vector<AttrID> inputAttrIDs;
    std::vector<AttrID> outputAttrIDs;
    // lastSeenVersions.size() must equal inputAttrIDs.size()
    std::vector<uint64_t> lastSeenVersions;

    NDESC bool isTombstone() const
    {
        return typeID == kInvalidNodeTypeID;
    }
    void invalidate()
    {
        typeID = kInvalidNodeTypeID;
        inputAttrIDs.clear();
        outputAttrIDs.clear();
        lastSeenVersions.clear();
    }
    // TODO: toString
};

struct AttributeDescriptor final
{
    const std::string_view name;
    const AttributeDataType type;
    const AttributeDirection direction;
    // TODO: toString
};

struct AttrInfo final
{
    AttrID attrID;
    AttributeDataType type;
};

/// Lightweight object that allows the node to read/write
/// to the proper places without need to know the details of how/where the data comes/goes.
class DataStore final
{
    const std::span<AttributeRecord> _attrs;
    const std::span<DataSlot> _data;

public:
    DataStore(const std::span<AttributeRecord> attrs, const std::span<DataSlot> dataSlots)
        : _attrs(attrs), _data(dataSlots)
    {}

    template<AttributeTypeConcept T>
    std::span<const T> read(const AttrID aid) const
    {
        const AttributeRecord& cRecord = _attrs[aid];
        if (cRecord.hasInputSources())
        {
            // IF there is a source, we need to view data from source
            const DataSlot& ds = _data[cRecord.upstream];
            return ds.readAsSpan<T>();
        }
        // IF no sources plugged in, then data slot in the same spot as attr ID
        const DataSlot& ds = _data[aid];
        return ds.readAsSpan<T>();
    }

    template<AttributeTypeConcept T>
    void write(const AttrID aid, const std::span<const T> data)
    {
        const AttributeRecord& cRecord = _attrs[aid];
        // we should never allow a write to input attribute that is plugged in.
        // The only node allowed to write to the upstream is the node owning the output attr.
        assert(not cRecord.hasInputSources());
        DataSlot& ds = _data[aid];
        ds.writeAsSpan<T>(data);
    }

    template<AttributeTypeConcept T>
    T readSingle(const AttrID aid)
    {
        const std::span<const T> value = read<T>(aid);
        assert(value.size() == 1);
        return value[0];
    }

    template<AttributeTypeConcept T>
    void writeSingle(const AttrID aid, const T value)
    {
        write<T>(aid, std::span<const T>{&value, 1});
    }
};

END_NAMESPACE