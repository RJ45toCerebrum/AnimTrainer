// Created by Tyler on 4/27/2026.
#pragma once

#include "ATSceneNode.h"
#include "Attributes/ATVec3Attribute.h"
#include "Attributes/ATFloatAttribute.h"

START_NAMESPACE(ATNode)

using ATScene::ATSceneNode;
using ATScene::NodeID;
using ATScene::NodeTypeID;
using ATAttribute::ATVec3Attribute;
using ATScene::AttributeDirection;
using ATScene::AttributePtr;
using ATScene::ATAttributeHandle;

class ATDebugSphereNode : public ATSceneNode
{
    ATAttributeHandle _outPositionHandle;
    ATAttributeHandle _inPositionHandle;
    ATAttributeHandle _inRadiusHandle;

public:
    ATDebugSphereNode(const NodeID ownerNodeID, const std::string_view& name) :
        ATSceneNode(ownerNodeID, name)
    {
        AttributePtr positionAttribute =
            std::make_unique<ATVec3Attribute>(ownerNodeID, false, AttributeDirection::Input);
        _inPositionHandle = registerInputAttribute(std::move(positionAttribute));

        AttributePtr radiusAttribute =
            std::make_unique<ATAttribute::ATFloatAttribute>(ownerNodeID, false, AttributeDirection::Input);
        _inRadiusHandle = registerInputAttribute(std::move(radiusAttribute));

        // this just acts as a pass-through for position...
        AttributePtr outputPositionAttr =
            std::make_unique<ATVec3Attribute>(ownerNodeID, false, AttributeDirection::Output);
        _outPositionHandle = registerOutputAttribute(std::move(outputPositionAttr));
    }

    NodeTypeID getNodeTypeID() const override
    {
        static constexpr NodeTypeID nodeTypeID = ATScene::atHashString("ATDebugSphereNode");
        return nodeTypeID;
    }

protected:
    void compute(const ATAttributeHandle ah) override
    {
        assert(ah == _outPositionHandle);
        const ATScene::AttributeData inPosition = getAttributeData(_inPositionHandle);
        const glm::vec3 pos = inPosition.getData<glm::vec3>();

        const ATScene::AttributeData inRadius = getAttributeData(_inRadiusHandle);
        const float radius = inRadius.getData<float>();

        const Vector3 p{pos.x, pos.y, pos.z};
        DrawSphere(p, radius, YELLOW);
    }
};

END_NAMESPACE