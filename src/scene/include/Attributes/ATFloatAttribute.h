// Created by Tyler on 4/27/2026.
#pragma once

#include "ATAttribute.h"

START_NAMESPACE(ATAttribute)

using ATScene::ATAttribute;
using ATScene::AttributeData;
using ATScene::NodeID;
using ATScene::AttributeDirection;

class ATFloatAttribute final : public ATAttribute
{
    std::vector<float> _data;

public:
    ATFloatAttribute(const NodeID owner, const bool isArray, const AttributeDirection direction) :
        ATAttribute(ATScene::AttributeDataType::Float, owner, isArray, direction)
    {}

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
        const AttributeData data(rawData, static_cast<int>(elementCount), ATScene::AttributeDataType::Float);
        return data;
    }
    bool setData(const AttributeData& attrData) override
    {
        if (not attrData.isValid())
            return false;

        assert(attrData.getDataType() == ATScene::AttributeDataType::Float);
        _data.clear();
        const std::span<const float> data = attrData.getDataArray<float>();
        for (const float v : data)
            _data.push_back(v);

        return true;
    }
};

END_NAMESPACE