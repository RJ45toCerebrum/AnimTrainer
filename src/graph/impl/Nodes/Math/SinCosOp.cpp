// Created by Tyler on 5/6/2026.
#include "Nodes/Math/SinCosOp.h"
#include <glm/glm.hpp>

START_NAMESPACE(ATGraph)

static constexpr std::string_view kRadiansAttrName("Radians");


void SinCosOp::compute(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID radiansInputAttrID = nodeRecord.inputAttrIDs.at(0);
    const AttrID sinOutputAttrID = nodeRecord.outputAttrIDs.at(0);
    const AttrID cosOutputAttrID = nodeRecord.outputAttrIDs.at(1);

    const auto radiansInputBuffer = dStore.read<float>(radiansInputAttrID);
    const std::size_t elementCount = radiansInputBuffer.size();

    auto sinBuffer = dStore.prepareWrite<float>(sinOutputAttrID, elementCount);
    auto cosBuffer = dStore.prepareWrite<float>(cosOutputAttrID, elementCount);
    for (int i = 0; i < elementCount; ++i)
    {
        const float radians = radiansInputBuffer[i];
        sinBuffer[i] = glm::sin(radians);
        cosBuffer[i] = glm::cos(radians);
    }
}

void SinCosOp::initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const
{
    assert(attrDescriptor.supportedTypes == AttributeDataType::Float);
    constexpr float defaultValue = 0.f;
    constexpr std::array defaultBuffer = {defaultValue};
    dataSlot.writeAsSpan<float>(defaultBuffer);
}

bool SinCosOp::changeAttributeDataType(const NodeRecord& nodeRecord,
    const AttributeDataType concreteType, const AttrID inputAttr, DataStore& dStore)
{
    dStore.updateAttributeType(inputAttr, concreteType);
    return true;
}

const std::span<const AttributeDescriptor> SinCosOp::inputAttrSchema() const
{
    static constexpr AttributeDescriptor kRadiansAttrDescriptor(
        kRadiansAttrName, AttributeDataType::Float, AttributeDirection::Input);
    static constexpr std::array kInputAttrs = {kRadiansAttrDescriptor};
    return kInputAttrs;
}

const std::span<const AttributeDescriptor> SinCosOp::outputAttrSchema() const
{
    static constexpr std::string_view kSinAttrName("Sin");
    static constexpr std::string_view kCosAttrName("Cos");

    static constexpr AttributeDescriptor kSinAttrDescriptor(
        kSinAttrName, AttributeDataType::Float, AttributeDirection::Output);
    static constexpr AttributeDescriptor kCosAttrDescriptor(
        kCosAttrName, AttributeDataType::Float, AttributeDirection::Output);

    static constexpr std::array outputAttrs = {kSinAttrDescriptor, kCosAttrDescriptor};
    return outputAttrs;
}

bool SinCosOp::alwaysCompute() const
{
    return false;
}
const std::string_view SinCosOp::nodeName() const
{
    return kNodeName;
}
const NodeTypeID SinCosOp::nodeTypeID() const
{
    return kNodeTypeID;
}

END_NAMESPACE
