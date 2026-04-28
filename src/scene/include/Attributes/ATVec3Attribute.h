// Created by Tyler on 4/27/2026.
#pragma once

#include "ATAttribute.h"

START_NAMESPACE(ATAttribute)

using ATScene::ATAttribute;
using ATScene::AttributeData;
using ATMath::Transform;
using ATScene::NodeID;
using ATScene::AttributeDirection;

class ATVec3Attribute final : public ATAttribute
{
    std::vector<glm::vec3> _data;

public:
    ATVec3Attribute(const NodeID owner, const bool isArray, const AttributeDirection direction) :
        ATAttribute(ATScene::AttributeDataType::Vec3, owner, isArray, direction)
    {}

protected:
    [[nodiscard]] int32_t getDataCount() const override
    {
        const std::size_t elementCount = _data.size();
        assert(elementCount < std::numeric_limits<int32_t>::max());
        return static_cast<int>(elementCount);
    }
    [[nodiscard]] AttributeData getRawData() const override
    {
        const void* rawData = static_cast<const void*>(_data.data());
        const std::size_t elementCount = _data.size();
        assert(elementCount < std::numeric_limits<int32_t>::max());
        const AttributeData data(rawData, static_cast<int>(elementCount), ATScene::AttributeDataType::Transform);
        return data;
    }
    bool setData(const AttributeData& attrData) override
    {
        if (not attrData.isValid())
            return false;

        assert(attrData.getDataType() == ATScene::AttributeDataType::Vec3);
        _data.clear();
        const std::span<const glm::vec3> data = attrData.getDataArray<glm::vec3>();
        for (const glm::vec3& v : data)
            _data.push_back(v);

        return true;
    }
};


END_NAMESPACE
