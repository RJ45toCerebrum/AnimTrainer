// Created by Tyler on 5/6/2026.
#include "Nodes/TimeNode.h"

START_NAMESPACE(ATGraph)

static constexpr std::string_view kOutputAttrName("TimeOutput");

void TimeNode::compute(const NodeRecord& nodeRecord, DataStore& dStore)
{
    assert(nodeRecord.outputAttrIDs.size() == 1);
    const auto appTime = static_cast<float>(GetTime());
    // TODO: delta time is from the last time a frame was drawn
    // This may not work in many cases. But will do for now.
    const auto deltaTime = static_cast<float>(GetFrameTime());
    const std::array data = {appTime, deltaTime};
    dStore.write<float>(nodeRecord.outputAttrIDs[0], data);
}

void TimeNode::initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const
{
    constexpr std::array kDefaultBufferValues = {0.0f, 0.0f};
    dataSlot.writeAsSpan<float>(kDefaultBufferValues);
}

NDESC bool TimeNode::changeAttributeDataType(const NodeRecord& nodeRecord, const AttributeDataType concreteType,
    const AttrID inputAttr, DataStore& dStore)
{
    return true;
}

// TimeNode's have 0 inputs
const std::span<const AttributeDescriptor> TimeNode::inputAttrSchema() const
{
    return {};
}

const std::span<const AttributeDescriptor> TimeNode::outputAttrSchema() const
{
    constexpr AttributeDescriptor outputDescriptor(kOutputAttrName, AttributeDataType::Float, AttributeDirection::Output);
    static constexpr std::array<const AttributeDescriptor, 1> kOutputAttrs = {outputDescriptor};
    return kOutputAttrs;
}

bool TimeNode::alwaysCompute() const
{
    return true;
}

const std::string_view TimeNode::nodeName() const
{
    return kNodeName;
}

const NodeTypeID TimeNode::nodeTypeID() const
{
    return kNodeTypeId;
}

END_NAMESPACE