// Created by Tyler on 4/29/2026.
#pragma once

#include "common.h"

#include <glm/vec3.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>
#include <memory_resource>
#include <format>

#include "ATMath.h"
#include "scene/include/ATAttribute.h"

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
    Int,
    Vec,
    Transform
};

// TODO: finish
consteval std::string_view attrDataTypeStr(const AttributeDataType type)
{
    static constexpr std::string_view kInvalidDataTypeStr = "InvalidDataType";
    static constexpr std::string_view kFloatAttrStr = "float";
    static constexpr std::string_view kIntAttrStr = "int";
    static constexpr std::string_view kVecAttrStr = "vec";
    static constexpr std::string_view kTransformStr = "transform";
    switch (type)
    {
        case AttributeDataType::Float:
            return kFloatAttrStr;
        case AttributeDataType::Int:
            return kIntAttrStr;
        case AttributeDataType::Vec:
            return kVecAttrStr;
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
    if constexpr (std::is_same_v<T, int>)
        return AttributeDataType::Int;
    else if constexpr (std::is_same_v<T, glm::vec2> or std::is_same_v<T, glm::vec3> or std::is_same_v<T, glm::vec4>)
        return AttributeDataType::Vec;
    else if constexpr (std::is_same_v<T, ATMath::Transform>)
        return AttributeDataType::Transform;

    throw std::exception("Type T not implemented yet");
}

/// TODO: finish
template<typename T>
concept AttributeTypeConcept =
    std::same_as<T, float> || std::same_as<T, int> ||
    std::same_as<T, glm::vec2> || std::same_as<T, glm::vec3> || std::same_as<T, glm::vec4>;

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
    // no code should ever rely on the order of this vector.
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

    NDESC auto findOutputSource(const AttrID attrID) const -> std::vector<unsigned>::const_iterator
    {
        assert(not isTombstone());
        if (not isOutputAttr())
            return downstream.end();

        // should never have upstream and downstream on same attr.
        assert(upstream == kInvalidAttr);
        const auto matchAttrID = [attrID](const AttrID downstreamInputAttrs) -> bool
        {
            return attrID == downstreamInputAttrs;
        };
        return std::ranges::find_if(downstream, matchAttrID);
    }

    void removeOutputSourceChecked(const AttrID inputAttrID)
    {
        assert(isOutputAttr());
        const auto fitr = findOutputSource(inputAttrID);
        assert(fitr != downstream.end());
        downstream.erase(fitr);
    }
    void invalidate()
    {
        owner = kInvalidNodeID;
        upstream = kInvalidAttr;
        downstream.clear();
    }
    // TODO: toString
};


// for every attribute there is a DataSlot entry in the graph. meaning,
// the length of the attribute record list and the length of graphs data slot list are equal
// at all times. On Node deletion, the memory is considered a tombstone.
// The internal bytes array memory will be reclaimed, but DataSlot in graph will not.
// Tombstone memory per node deletion =
// (Total attribute count for type) * sizeof(AttributeRecord) +
// (Total attribute count for type) * sizeof(DataSlot) +
// sizeof(NodeRecord)
// EXAMPLE:
// Lets say average attr count = 6
// sizeof(AttributeRecord) = 48 ; sizeof(DataSlot) = 40 ; sizeof(NodeRecord) = 104
// 6 * 48 + 6 * 40 + 104 = 632 bytes of wasted space per node deletion (assuming all internal vectors are reclaimed).
// However, when scene reloaded, space will be reclaimed. so not too big a deal.
// This makes deletion and (undo) much easier and less error prone. So I go with tombstone approach.
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
        assert(bytes.size() % sizeof(T) == 0);
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
        assert(bytes.size() % sizeof(T) == 0);
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
        assert(data.size_bytes() % sizeof(T) == 0);
        return {rawData, data.size_bytes() / sizeof(T)};
    }
};


// TODO: make all fields of the NodeRecord const (except for lastSeenVersions) and re-work graph createNode method.
// We require inputAttrIDs and outputAttrIDs to stay fixed for the lifetime of NodeRecord.
/// Every created node has a NodeRecord
/// typeID => INodeCompute
struct NodeRecord final
{
    // the nodes ID is its index into graphs node records array. No need to store here.
    NodeTypeID typeID = kInvalidNodeTypeID;
    // important: inputAttrIDs & outputAttrIDs vectors MUST stay fixed.
    // This is because data can beread/write using the indices of these arrays.
    std::vector<AttrID> inputAttrIDs;
    std::vector<AttrID> outputAttrIDs;
    // lastSeenVersions.size() must stay equal to inputAttrIDs.size()
    std::vector<uint64_t> lastSeenVersions;

    NDESC bool isTombstone() const
    {
        return typeID == kInvalidNodeTypeID;
    }
    NDESC int getAttrIndex(const AttrID attrID) const
    {
        assert(not isTombstone());
        for (int i = 0; i < inputAttrIDs.size(); ++i)
        {
            if (inputAttrIDs[i] == attrID)
                return i;
        }
        return -1;
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