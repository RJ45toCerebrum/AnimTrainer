// Created by Tyler on 4/29/2026.
#pragma once

#include "common.h"

#include <glm/vec3.hpp>

#include <type_traits>
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

enum class AttributeDataType : uint64_t
{
    Bool            = 1 << 0,
    Float           = 1 << 1,
    Int             = 1 << 2,
    Vec2            = 1 << 3,
    Vec3            = 1 << 4,
    Vec4            = 1 << 5,
    Quaternion      = 1 << 6,
    Transform       = 1 << 7
};

constexpr AttributeDataType operator|(const AttributeDataType lhs, const AttributeDataType rhs)
{
    const auto lhsAttrID = static_cast<uint64_t>(lhs);
    const auto rhsAttrID = static_cast<uint64_t>(rhs);
    return static_cast<AttributeDataType>(lhsAttrID | rhsAttrID);
}

inline bool isConcreteType(const AttributeDataType attrType)
{
    const auto attrTypeFlags = static_cast<uint64_t>(attrType);
    return (attrTypeFlags & (attrTypeFlags - 1)) == 0;
}

inline bool isConcreteTypeSupported(const AttributeDataType supportedTypeFlags, const AttributeDataType concreteType)
{
    const auto typeFlags = static_cast<uint64_t>(supportedTypeFlags);
    const auto attrTypeFlags = static_cast<uint64_t>(concreteType);
    assert(isConcreteType(concreteType) and
        "[isTypeSupported] attrType must only have one flag set to be valid");
    return (typeFlags & attrTypeFlags) != 0;
}

// TODO: finish
constexpr std::string_view attrDataTypeStr(const AttributeDataType type)
{
    static constexpr std::string_view kInvalidDataTypeStr = "InvalidDataType";
    static constexpr std::string_view kMultiTypeStr = "Multi-Type";
    static constexpr std::string_view kBoolAttrStr = "bool";
    static constexpr std::string_view kFloatAttrStr = "float";
    static constexpr std::string_view kIntAttrStr = "int";
    static constexpr std::string_view kVec2AttrStr = "vec2";
    static constexpr std::string_view kVec3AttrStr = "vec3";
    static constexpr std::string_view kVec4AttrStr = "vec4";
    static constexpr std::string_view kQuaternionStr = "quaternion";
    static constexpr std::string_view kTransformStr = "transform";
    switch (type)
    {
        case AttributeDataType::Bool:
            return kBoolAttrStr;
        case AttributeDataType::Float:
            return kFloatAttrStr;
        case AttributeDataType::Int:
            return kIntAttrStr;
        case AttributeDataType::Vec2:
            return kVec2AttrStr;
        case AttributeDataType::Vec3:
            return kVec3AttrStr;
        case AttributeDataType::Vec4:
            return kVec4AttrStr;
        case AttributeDataType::Quaternion:
            return kQuaternionStr;
        case AttributeDataType::Transform:
            return kTransformStr;
        default:
            return isConcreteType(type) ? kInvalidDataTypeStr : kMultiTypeStr;
    }
}

/// TODO: finish
template<typename T>
consteval AttributeDataType enumFromAttrType()
{
    if constexpr (std::is_same_v<T, bool>)
        return AttributeDataType::Bool;
    if constexpr (std::is_same_v<T, float>)
        return AttributeDataType::Float;
    if constexpr (std::is_same_v<T, int>)
        return AttributeDataType::Int;
    if constexpr (std::is_same_v<T, glm::vec2>)
        return AttributeDataType::Vec2;
    if constexpr (std::is_same_v<T, glm::vec3>)
        return AttributeDataType::Vec3;
    if constexpr (std::is_same_v<T, glm::vec4>)
        return AttributeDataType::Vec4;
    if constexpr (std::is_same_v<T, glm::quat>)
        return AttributeDataType::Quaternion;
    if constexpr (std::is_same_v<T, ATMath::Transform>)
        return AttributeDataType::Transform;

    throw std::exception("Type T not implemented yet");
}

/// TODO: finish
template<typename T>
concept AttributeTypeConcept =
    std::same_as<T, bool> || std::same_as<T, float> || std::same_as<T, int> ||
    std::same_as<T, glm::vec2> || std::same_as<T, glm::vec3> || std::same_as<T, glm::vec4>;

template <AttributeTypeConcept T>
void assertAlignment(const std::vector<std::byte>& buffer)
{
    if (buffer.empty())
        return;
    const auto addr = reinterpret_cast<std::uintptr_t>(buffer.data());
    assert(addr % alignof(T) == 0 and "Vector data is misaligned for the requested type.");
}

inline std::size_t sizeFromType(const AttributeDataType DT)
{
    assert(isConcreteType(DT));
    if (DT == AttributeDataType::Bool)
        return sizeof(bool);
    if (DT == AttributeDataType::Float)
        return sizeof(float);
    if (DT == AttributeDataType::Int)
        return sizeof(int);
    if (DT == AttributeDataType::Vec2)
        return sizeof(glm::vec2);
    if (DT == AttributeDataType::Vec3)
        return sizeof(glm::vec3);
    if (DT == AttributeDataType::Vec4)
        return sizeof(glm::vec4);
    if (DT == AttributeDataType::Quaternion)
        return sizeof(glm::quat);
    if (DT == AttributeDataType::Transform)
        return sizeof(ATMath::Transform);

    throw std::exception("Type T not implemented yet");
}

inline void writeDefaultValue(const AttributeDataType dataType, const std::span<std::byte> byteBuffer)
{
    const std::size_t size = sizeFromType(dataType);
    assert(byteBuffer.size_bytes() == size);
    switch (dataType)
    {
        case AttributeDataType::Bool:
        {
            constexpr bool defaultValue = false;
            std::memcpy(byteBuffer.data(), &defaultValue, size);
            break;
        }
        case AttributeDataType::Float:
        {
            constexpr float defaultValueFloat = 0.0f;
            std::memcpy(byteBuffer.data(), &defaultValueFloat, size);
            break;
        }
        case AttributeDataType::Int:
        {
            constexpr int defaultValueFloat = 0;
            std::memcpy(byteBuffer.data(), &defaultValueFloat, size);
            break;
        }
        case AttributeDataType::Vec2:
        {
            constexpr glm::vec2 defaultValueFloat(0.0f,0.0f);
            std::memcpy(byteBuffer.data(), &defaultValueFloat, size);
            break;
        }
        case AttributeDataType::Vec3:
        {
            constexpr glm::vec3 defaultValueFloat(0.0f,0.0f,0.0f);
            std::memcpy(byteBuffer.data(), &defaultValueFloat, size);
            break;
        }
        case AttributeDataType::Vec4:
        {
            constexpr glm::vec4 defaultValueFloat(0.0f,0.0f,0.0f,0.0f);
            std::memcpy(byteBuffer.data(), &defaultValueFloat, size);
            break;
        }
        case AttributeDataType::Quaternion:
        {
            constexpr glm::quat identity = glm::quat_identity<float, glm::qualifier::defaultp>();
            std::memcpy(byteBuffer.data(), &identity, size);
            break;
        }
        default:
            throw std::exception("Type T not implemented yet");
    }
}



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
    AttributeDataType supportedTypes = AttributeDataType::Float;
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
        return not downstream.empty();
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
        return not downstream.empty();
    }
    NDESC bool supportsConcreteType(const AttributeDataType concreteType) const
    {
        return isConcreteTypeSupported(supportedTypes, concreteType);
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
            // there should never be invalid attr ids in downstream
            assert(downstreamInputAttrs != kInvalidAttr);
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
    const AttributeDataType supportedTypes;
    const AttributeDirection direction;
    // TODO: toString
};

struct AttrInfo final
{
    AttrID attrID;
    AttributeDataType supportedTypes;
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
class DataSlot final
{
    std::vector<std::byte> _bytes;
    // this version flag is key to efficiency of the graph.
    // If there is a mismatch between this value and lastSeenVersions this means the graph needs recompute.
    uint64_t _version = 0;
    // this represents the actual data being stored. It must have only one flag set to be considered valid.
    AttributeDataType _concreteType = AttributeDataType::Bool;

public:
    // we can change the type of the stored data.
    // However, this must be done very carefully. No connections to the node are allowed to do type changes.
    // Review ATGraph for more details. Nodes should call this in initDataSlotDefaultValue
    void updateConcreteType(const AttributeDataType concreteType)
    {
        assert(isConcreteType(concreteType) and
            "[DataSlot] concrete type must only have one flag set to be valid");
        _concreteType = concreteType;
        _bytes.clear();
        _bytes.resize(sizeFromType(_concreteType));
        writeDefaultValue(_concreteType, _bytes);
    }

    void updateVersion(const uint64_t version)
    {
        _version = version;
    }

    void initDefaultValue()
    {
        _bytes.resize(sizeFromType(_concreteType));
        writeDefaultValue(_concreteType, _bytes);
    }

    NDESC AttributeDataType getConcreteType() const
    {
        return _concreteType;
    }

    NDESC uint64_t version() const
    {
        return _version;
    }

    NDESC std::size_t bytesAllocated() const
    {
        return _bytes.size();
    }

    void freeMemory()
    {
        _bytes.clear();
        _bytes.shrink_to_fit();
    }


    template<AttributeTypeConcept T>
    std::span<const T> readAsSpan() const
    {
        assertAlignment<T>(_bytes);
        assert(_concreteType == enumFromAttrType<T>());
        assert(_bytes.size() % sizeof(T) == 0);
        const T* rawData = reinterpret_cast<const T*>(_bytes.data());
        return {rawData, _bytes.size() / sizeof(T)};
    }

    NDESC std::span<const std::byte> readAsBytes() const
    {
        // no partial data
        assert(_bytes.size() % sizeFromType(_concreteType) == 0);
        return {_bytes.data(), _bytes.size()};
    }

    template<AttributeTypeConcept T>
    void writeAsSpan(const std::span<const T> values)
    {
        assert(_concreteType == enumFromAttrType<T>());
        assert(_bytes.size() % sizeof(T) == 0);
        const auto* begin = reinterpret_cast<const std::byte*>(values.data());
        _bytes.assign(begin, begin + values.size_bytes());
        ++_version;
    }

    void writeRawBytes(const std::span<const std::byte> data)
    {
        _bytes.assign(data.begin(), data.end());
        _version++;
    }

    // TODO: add unit test for this as this is on the sketchy side.
    template<AttributeTypeConcept T>
    std::span<T> prepareWrite(const size_t count)
    {
        assertAlignment<T>(_bytes);
        assert(_concreteType == enumFromAttrType<T>());
        _bytes.resize(count * sizeof(T));
        ++_version;
        return {reinterpret_cast<T*>(_bytes.data()), count};
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

    // This can be used by node compute method to write directly into the buffer rather than
    // having to allocate it in compute. Avoiding an allocation just to copy into a DataSlot buffer.
    // NOTE: this increments the version, so once you request this data you are committed to writing to the buffer.
    template<AttributeTypeConcept T>
    std::span<T> prepareWrite(const AttrID aid, const size_t count)
    {
        DataSlot& ds = _data[aid];
        return ds.prepareWrite<T>(count);
    }

    void updateAttributeType(const AttrID aid, const AttributeDataType dataType) const
    {
        // graph should have verified the type was supported before getting here.
        assert(isConcreteTypeSupported(_attrs[aid].supportedTypes, dataType));
        DataSlot& ds = _data[aid];
        ds.updateConcreteType(dataType);
    }

    void initDefaultValue(const AttrID aid) const
    {
        DataSlot& ds = _data[aid];
        ds.initDefaultValue();
    }

    NDESC AttributeDataType getConcreteType(const AttrID aid) const
    {
        return _data[aid].getConcreteType();
    }
};

END_NAMESPACE