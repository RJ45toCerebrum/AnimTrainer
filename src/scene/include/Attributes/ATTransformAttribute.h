// Created by Tyler on 4/27/2026.
#pragma once

#include "ATAttribute.h"

START_NAMESPACE(ATAttribute)

using ATScene::ATAttribute;
using ATScene::AttributeData;
using ATMath::Transform;
using ATScene::NodeID;
using ATScene::AttributeDirection;

class ATTransformAttribute final : public ATAttribute
{
    std::vector<Transform> _transformData;

public:
    ATTransformAttribute(const NodeID owner, const bool isArray, const AttributeDirection direction) :
        ATAttribute(ATScene::AttributeDataType::Transform, owner, isArray, direction)
    {}

protected:
    [[nodiscard]] int32_t getDataCount() const override
    {
        const std::size_t elementCount = _transformData.size();
        assert(elementCount < std::numeric_limits<int32_t>::max());
        return static_cast<int>(elementCount);
    }
    [[nodiscard]] AttributeData getRawData() const override
    {
        const void* rawTransformData = static_cast<const void*>(_transformData.data());
        const std::size_t elementCount = _transformData.size();
        assert(elementCount < std::numeric_limits<int32_t>::max());
        const AttributeData data(
            rawTransformData, static_cast<int>(elementCount), ATScene::AttributeDataType::Transform);
        return data;
    }
    bool setData(const AttributeData& attrData) override
    {
        if (not attrData.isValid())
            return false;

        assert(attrData.getDataType() == ATScene::AttributeDataType::Transform);
        _transformData.clear();
        const std::span<const Transform> data = attrData.getDataArray<ATMath::Transform>();
        for (const Transform& transform : data)
            _transformData.push_back(transform);

        return true;
    }
};

END_NAMESPACE