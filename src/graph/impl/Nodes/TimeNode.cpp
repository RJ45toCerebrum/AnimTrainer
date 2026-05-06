// Created by Tyler on 5/6/2026.
#include "Nodes/TimeNode.h"

START_NAMESPACE(ATGraph)

void TimeNode::compute(const NodeRecord& nodeRecord, DataStore& dStore)
{
    assert(nodeRecord.outputAttrIDs.size() == 1);
    const auto appTime = static_cast<float>(GetTime());
    // TODO: delta time is from the last time a frame was drawn
    // This may not work in many cases. But will do for now.
    const auto deltaTime = static_cast<float>(GetFrameTime());
    const std::array data = {appTime, deltaTime};
    const std::span rawData(data.data(), data.size());
    dStore.write(nodeRecord.outputAttrIDs[0], rawData);
}

void TimeNode::initDataSlotDefaultValue(DataSlot& dataSlot, const AttributeDescriptor& attrDescriptor) const
{}

// TimeNode's have 0 inputs
constexpr std::span<const AttributeDescriptor> TimeNode::inputAttrSchema() const
{
    return {};
}

constexpr std::span<const AttributeDescriptor> TimeNode::outputAttrSchema() const
{
    constexpr AttributeDescriptor outputDescriptor("TimeOutput", AttributeDataType::Float, AttributeDirection::Output);
    static constexpr std::array<const AttributeDescriptor, 1> outputAttrs = {outputDescriptor};
    return outputAttrs;
}

bool TimeNode::alwaysCompute() const
{
    return true;
}

constexpr std::string_view TimeNode::nodeName() const
{
    return kNodeName;
}

constexpr NodeTypeID TimeNode::nodeTypeID() const
{
    return kNodeTypeId;
}

END_NAMESPACE