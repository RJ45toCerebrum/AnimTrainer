// Created by Tyler on 5/1/2026.
#pragma once

#include "NodeCompute.h"
#include <array>

START_NAMESPACE(ATGraph)

class AddNode final : public INodeCompute
{
    static constexpr uint16_t kLeftAttrIndexer = 0;
    static constexpr std::string_view kLeftAttrName = "Left";
    static constexpr uint16_t kRightAttrIndexer = 1;
    static constexpr std::string_view kRightAttrName = "Right";

public:
    static constexpr std::string_view kNodeName = "AddNode";
    static constexpr NodeTypeID kNodeTypeId = nodeTypeHash(kNodeName);

    void compute(const NodeRecord& nodeRecord, DataStore& dStore) override
    {
        assert(nodeRecord.inputAttrIDs.size() == 2);
        assert(nodeRecord.outputAttrIDs.size() == 1);
        // read left attr
        const AttrID leftAttr = nodeRecord.inputAttrIDs[kLeftAttrIndexer];
        const AttrID rightAttr = nodeRecord.inputAttrIDs[kRightAttrIndexer];

        const auto leftFloat = dStore.readSingle<float>(leftAttr);
        const auto rightFloat = dStore.readSingle<float>(rightAttr);
        const float result = leftFloat + rightFloat;

        const AttrID outputAttrID = nodeRecord.outputAttrIDs[0];
        dStore.writeSingle<float>(outputAttrID, result);
    }

    NDESC constexpr std::span<const AttributeDescriptor> inputAttrSchema() const override
    {
        static constexpr AttributeDescriptor kLeftFloatAttrDesc(kLeftAttrName, AttributeDataType::Float, AttributeDirection::Input);
        static constexpr AttributeDescriptor kRightFloatAttrDesc(kRightAttrName, AttributeDataType::Float, AttributeDirection::Input);
        static constexpr std::array<const AttributeDescriptor, 2> inputAttrs = {kLeftFloatAttrDesc, kRightFloatAttrDesc};
        return inputAttrs;
    }

    NDESC constexpr std::span<const AttributeDescriptor> outputAttrSchema() const override
    {
        static constexpr AttributeDescriptor kOutFloatAttrDesc("Out", AttributeDataType::Float, AttributeDirection::Output);
        static constexpr std::array<const AttributeDescriptor, 1> outputAttrs = {kOutFloatAttrDesc};
        return outputAttrs;
    }

    NDESC constexpr std::string_view nodeName() const override
    {
        return kNodeName;
    }

    NDESC constexpr NodeTypeID nodeTypeID() const override
    {
        return kNodeTypeId;
    }
};

END_NAMESPACE
