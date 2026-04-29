// Created by Tyler on 4/28/2026.
#pragma once

#include <common.h>
#include "Attributes/ATFloatAttribute.h"
#include "ATSceneGraph.h"

START_NAMESPACE(ATNode)

using ATScene::ATSceneNode;
using ATScene::NodeID;
using ATScene::NodeTypeID;
using ATScene::AttributeDirection;
using ATScene::AttributePtr;
using ATScene::ATAttributeHandle;
using FloatAttr = ATAttribute::ATFloatAttribute;
using AttrData = ATScene::AttributeData;
using ATScene::ISceneNodeFactory;


class TimeNode final : public ATSceneNode
{
    // in and out params store two floats => [delta Time, Game Elapsed Time]
    ATAttributeHandle _inTimeAttr;
    ATAttributeHandle _outTimeAttr;

public:
    static constexpr NodeTypeID nodeTypeID = ATScene::atHashString("TimeNode");

    TimeNode(const NodeID ownerNodeID, const std::string_view& name) :
        ATSceneNode(ownerNodeID, name)
    {
        AttributePtr inTimeAttrPtr = std::make_unique<FloatAttr>(ownerNodeID, true, AttributeDirection::Input);
        _inTimeAttr = registerInputAttribute(std::move(inTimeAttrPtr));

        AttributePtr outTimeAttrPtr = std::make_unique<FloatAttr>(ownerNodeID, true, AttributeDirection::Output);
        _outTimeAttr = registerOutputAttribute(std::move(outTimeAttrPtr));
    }

    [[nodiscard]] NodeTypeID getNodeTypeID() const override
    {
        return nodeTypeID;
    }

protected:
    void compute(const ATAttributeHandle ah) override
    {
        assert(ah == _outTimeAttr);
        const auto inTimeData = getAttributeData(_inTimeAttr);
        const std::span<const float> timeData = inTimeData.getDataArray<float>();
        assert(static_cast<int>(timeData.size()) == 2);

        // Time node simply acts as a pass-through node.
        // This allows us to trigger downstream node compute for nodes that need compute every frame without
        // needing special update logic for the graph.
        ATScene::ATAttribute& outAttrRef = getAttributeRef(_outTimeAttr);
        outAttrRef.setData(inTimeData);
    }
};

class TimeNodeFactory final : public ISceneNodeFactory
{
public:
    TimeNodeFactory() = default;

    std::unique_ptr<ATSceneNode> createSceneNode(NodeID newNodeID, const std::string_view& name) override
    {
        std::unique_ptr<ATSceneNode> newNode = std::make_unique<TimeNode>(newNodeID, name);
        return newNode;
    }
    [[nodiscard]] NodeTypeID getNodeTypeID() const override
    {
        return TimeNode::nodeTypeID;
    }
    static void registerNodeFactory()
    {
        std::unique_ptr<ISceneNodeFactory> factoryPtr = std::make_unique<TimeNodeFactory>();
        ATScene::ATSceneGraph::registerNodeType(TimeNode::nodeTypeID, std::move(factoryPtr));
    }
};

END_NAMESPACE