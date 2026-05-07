// Created by Tyler on 5/6/2026.
#include "Nodes/Math/SinCosOp.h"
#include <glm/glm.hpp>

START_NAMESPACE(ATGraph)

static constexpr std::string_view kRadiansAttrName("Radians");

void SinCosOp::compute(const NodeRecord& nodeRecord, DataStore& dStore)
{
    const AttrID inputAttrID = nodeRecord.inputAttrIDs.at(0);
    const auto radians = dStore.readSingle<float>(inputAttrID);
    const AttrID sinOutputAttrID = nodeRecord.outputAttrIDs.at(0);
    const AttrID cosOutputAttrID = nodeRecord.outputAttrIDs.at(1);
    dStore.writeSingle<float>(sinOutputAttrID, glm::sin(radians));
    dStore.writeSingle<float>(cosOutputAttrID, glm::cos(radians));
}

void SinCosOp::initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const
{
    constexpr float defaultValue = 0.f;
    const std::span inputAttrData(&defaultValue, 1);
    dataSlot.writeAsSpan(inputAttrData);
}

constexpr std::span<const AttributeDescriptor> SinCosOp::inputAttrSchema() const
{
    static constexpr AttributeDescriptor kRadiansAttrDescriptor(
        kRadiansAttrName, AttributeDataType::Float, AttributeDirection::Input);
    static constexpr std::array inputAttrs = {kRadiansAttrDescriptor};
    return inputAttrs;
}

constexpr std::span<const AttributeDescriptor> SinCosOp::outputAttrSchema() const
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

constexpr std::string_view SinCosOp::nodeName() const
{
    return kNodeName;
}

constexpr NodeTypeID SinCosOp::nodeTypeID() const
{
    return kNodeTypeID;
}

END_NAMESPACE
