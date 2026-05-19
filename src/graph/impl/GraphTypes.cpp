// Created by Tyler on 5/18/2026.
#include "GraphTypes.h"
#include "glm/gtx/quaternion.hpp"
#include <iostream>

START_NAMESPACE(ATGraph)

bool isConcreteType(const AttributeDataType attrType)
{
    if (hasInvalidFlag(attrType))
        return false;
    const auto attrTypeFlags = static_cast<uint64_t>(attrType);
    return (attrTypeFlags & (attrTypeFlags - 1)) == 0;
}

bool isConcreteTypeSupported(const AttributeDataType supportedTypeFlags, const AttributeDataType concreteType)
{
    if (hasInvalidFlag(supportedTypeFlags) or hasInvalidFlag(concreteType))
        return false;
    assert(isConcreteType(concreteType) and
        "[isTypeSupported] attrType must only have one flag set to be valid");
    const auto typeFlags = static_cast<uint64_t>(supportedTypeFlags);
    const auto attrTypeFlags = static_cast<uint64_t>(concreteType);
    return (typeFlags & attrTypeFlags) != 0;
}

std::size_t sizeFromType(const AttributeDataType DT)
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

void writeDefaultValue(const AttributeDataType dataType, const std::span<std::byte> byteBuffer)
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
        case AttributeDataType::Invalid:
            throw std::exception("Invalid flag set");
        default:
            throw std::exception("Type T not implemented yet");
    }
}


bool AttributeRecord::isTombstone() const
{
    return owner == kInvalidNodeID;
}
NDESC bool AttributeRecord::isInputAttr() const
{
    assert(not isTombstone());
    return direction == AttributeDirection::Input;
}
NDESC bool AttributeRecord::isOutputAttr() const
{
    assert(not isTombstone());
    return direction == AttributeDirection::Output;
}
NDESC bool AttributeRecord::hasSources() const
{
    assert(not isTombstone());
    if (direction == AttributeDirection::Input)
        return upstream != kInvalidAttr;
    return not downstream.empty();
}
NDESC bool AttributeRecord::hasInputSources() const
{
    assert(not isTombstone());
    if (not isInputAttr())
        return false;
    // an input attribute should never downstream plugs
    assert(downstream.empty());
    return upstream != kInvalidAttr;
}
NDESC bool AttributeRecord::hasOutputSources() const
{
    assert(not isTombstone());
    if (not isOutputAttr())
        return false;
    // an output attribute should never have upstream plugged
    assert(upstream == kInvalidAttr);
    return not downstream.empty();
}
NDESC bool AttributeRecord::supportsConcreteType(const AttributeDataType concreteType) const
{
    return isConcreteTypeSupported(supportedTypes, concreteType);
}
NDESC auto AttributeRecord::findOutputSource(const AttrID attrID) const -> std::vector<unsigned>::const_iterator
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
void AttributeRecord::removeOutputSourceChecked(const AttrID inputAttrID)
{
    assert(isOutputAttr());
    const auto fitr = findOutputSource(inputAttrID);
    assert(fitr != downstream.end());
    downstream.erase(fitr);
}
void AttributeRecord::invalidate()
{
    owner = kInvalidNodeID;
    upstream = kInvalidAttr;
    downstream.clear();
    downstream.shrink_to_fit();
    supportedTypes = AttributeDataType::Invalid;
}


NDESC bool NodeRecord::isTombstone() const
{
    return typeID == kInvalidNodeTypeID;
}
NDESC int NodeRecord::getAttrIndex(const AttrID attrID) const
{
    assert(not isTombstone());
    for (int i = 0; i < inputAttrIDs.size(); ++i)
    {
        if (inputAttrIDs[i] == attrID)
            return i;
    }
    return -1;
}
void NodeRecord::invalidate()
{
    typeID = kInvalidNodeTypeID;
    inputAttrIDs.clear();
    inputAttrIDs.shrink_to_fit();
    outputAttrIDs.clear();
    outputAttrIDs.shrink_to_fit();
    lastSeenVersions.clear();
    lastSeenVersions.shrink_to_fit();
}


// we can change the type of the stored data.
// However, this must be done very carefully. No connections to the node are allowed to do type changes.
// Review ATGraph for more details. Nodes should call this in initDataSlotDefaultValue
void DataSlot::updateConcreteType(const AttributeDataType concreteType)
{
    assert(isConcreteType(concreteType) and
        "[DataSlot] concrete type must only have one flag set to be valid and must not have Invalid flag set");
    _concreteType = concreteType;
    _bytes.clear();
    _bytes.resize(sizeFromType(_concreteType));
    writeDefaultValue(_concreteType, _bytes);
}
void DataSlot::updateVersion(const uint64_t version)
{
    _version = version;
}
void DataSlot::initDefaultValue()
{
    _bytes.resize(sizeFromType(_concreteType));
    writeDefaultValue(_concreteType, _bytes);
}
void DataSlot::writeRawBytes(const std::span<const std::byte> data)
{
    assert(data.size() % sizeFromType(_concreteType) == 0 and
        "[DataSlot::writeRawBytes] attempting to write partial data");
    _bytes.assign(data.begin(), data.end());
    _version++;
}
NDESC AttributeDataType DataSlot::getConcreteType() const
{
    return _concreteType;
}
NDESC uint64_t DataSlot::version() const
{
    return _version;
}
NDESC std::size_t DataSlot::bytesAllocated() const
{
    return _bytes.size();
}
NDESC std::size_t DataSlot::elementCount() const
{
    const auto elementSize = sizeFromType(_concreteType);
    assert(bytesAllocated() % elementSize == 0);
    return bytesAllocated() / sizeFromType(_concreteType);
}
void DataSlot::freeMemory()
{
    // NOTE: we leave _concreteType; this omission is important because the graph does not consider upstream
    // when checking attribute types. It just looks directly into attributes corresponding data slot type.
    // Enforcing the input and output attribute types are always equal is a simplification.
    // Therefore, when claming bytes, we leave _concreteType
    _bytes.clear();
    _bytes.shrink_to_fit();
}

// NOTE: when writing data, we should only allow writes to output attributes
// OR UN-plugged input attributes. The DataStore takes care of this logic.

void DataStore::updateAttributeType(const AttrID aid, const AttributeDataType dataType) const
{
    // graph should have verified the type was supported before getting here.
    assert(isConcreteTypeSupported(_attrs[aid].supportedTypes, dataType));
    const AttributeRecord& cRecord = _attrs[aid];
    if (cRecord.hasInputSources())
    {
        std::cerr << "[DataStore::updateAttributeType] attempting to update the attribute "
                     "type of a plugged attribute" << std::endl;
        return;
    }
    DataSlot& ds = _data[aid];
    ds.updateConcreteType(dataType);
}
void DataStore::writeRawBytes(const AttrID aid, const std::span<const std::byte> data) const
{
    const AttributeRecord& cRecord = _attrs[aid];
    if (cRecord.hasInputSources())
    {
        std::cerr << "[DataStore::updateAttributeType] attempting to write data into an input attribute "
             "that has an upstream source" << std::endl;
        return;
    }
    DataSlot& ds = _data[aid];
    ds.writeRawBytes(data);
}
void DataStore::initDefaultValue(const AttrID aid) const
{
    const AttributeRecord& cRecord = _attrs[aid];
    if (cRecord.hasInputSources())
    {
        std::cerr << "[DataStore::updateAttributeType] attempting to write data into an input attribute "
             "that has an upstream source" << std::endl;
        return;
    }
    DataSlot& ds = _data[aid];
    ds.initDefaultValue();
}
NDESC AttributeDataType DataStore::getConcreteType(const AttrID aid) const
{
    // no need to go upstream as we ensure this and upstream data slot types are always equal.
    // However, for now I want to assert this to be true for my sanity.
    const AttributeDataType dataType = _data[aid].getConcreteType();
    const AttributeRecord& cRecord = _attrs[aid];
    if (cRecord.hasInputSources())
    {
        const DataSlot& upstreamDataSlot = _data[cRecord.upstream];
        assert(upstreamDataSlot.getConcreteType() == dataType);
    }
    return dataType;
}
NDESC std::size_t DataStore::elementCount(const AttrID aid) const
{
    const AttributeRecord& cRecord = _attrs[aid];
    if (cRecord.hasInputSources())
    {
        const DataSlot& upstreamDataSlot = _data[cRecord.upstream];
        return upstreamDataSlot.elementCount();
    }
    return _data[aid].elementCount();
}
NDESC const AttributeRecord& DataStore::getAttributeRecord(const AttrID aid) const
{
    return _attrs[aid];
}
bool DataStore::doesAttrHaveInputSource(const AttrID aid) const
{
    const AttributeRecord& cRecord = _attrs[aid];
    return cRecord.hasInputSources();
}
bool DataStore::doesAttrHaveConnection(const AttrID aid) const
{
    const AttributeRecord& cRecord = _attrs[aid];
    return cRecord.hasSources();
}

END_NAMESPACE
