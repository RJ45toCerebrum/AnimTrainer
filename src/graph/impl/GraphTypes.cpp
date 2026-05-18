// Created by Tyler on 5/18/2026.
#include "GraphTypes.h"

START_NAMESPACE(ATGraph)

bool isConcreteType(const AttributeDataType attrType)
{
    const auto attrTypeFlags = static_cast<uint64_t>(attrType);
    return (attrTypeFlags & (attrTypeFlags - 1)) == 0;
}

bool isConcreteTypeSupported(const AttributeDataType supportedTypeFlags, const AttributeDataType concreteType)
{
    const auto typeFlags = static_cast<uint64_t>(supportedTypeFlags);
    const auto attrTypeFlags = static_cast<uint64_t>(concreteType);
    assert(isConcreteType(concreteType) and
        "[isTypeSupported] attrType must only have one flag set to be valid");
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
    outputAttrIDs.clear();
    lastSeenVersions.clear();
}


// we can change the type of the stored data.
// However, this must be done very carefully. No connections to the node are allowed to do type changes.
// Review ATGraph for more details. Nodes should call this in initDataSlotDefaultValue
void DataSlot::updateConcreteType(const AttributeDataType concreteType)
{
    assert(isConcreteType(concreteType) and
        "[DataSlot] concrete type must only have one flag set to be valid");
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
void DataSlot::freeMemory()
{
    _bytes.clear();
    _bytes.shrink_to_fit();
}


void DataStore::updateAttributeType(const AttrID aid, const AttributeDataType dataType) const
{
    // graph should have verified the type was supported before getting here.
    assert(isConcreteTypeSupported(_attrs[aid].supportedTypes, dataType));
    DataSlot& ds = _data[aid];
    ds.updateConcreteType(dataType);
}
void DataStore::initDefaultValue(const AttrID aid) const
{
    DataSlot& ds = _data[aid];
    ds.initDefaultValue();
}
NDESC AttributeDataType DataStore::getConcreteType(const AttrID aid) const
{
    return _data[aid].getConcreteType();
}

END_NAMESPACE