// Created by Tyler on 5/19/2026.
#pragma once

#include "GraphTypes.h"

START_NAMESPACE(ATGraph)

/// Avoid using when/if at all possible as this results in instantiation
/// of templates for each attribute type. Which will yield longer compile times.
/// Prefer the use of narrower handlers such as attributeVecTypeHandler
template<typename F>
decltype(auto) attributeGenericTypeHandler(const AttributeDataType dt, F&& f)
{
    switch (dt)
    {
        case AttributeDataType::Bool:
            return f.template operator()<bool>();
        case AttributeDataType::Float:
            return f.template operator()<float>();
        case AttributeDataType::Int:
            return f.template operator()<int>();
        case AttributeDataType::Vec2:
            return f.template operator()<glm::vec2>();
        case AttributeDataType::Vec3:
            return f.template operator()<glm::vec3>();
        case AttributeDataType::Vec4:
            return f.template operator()<glm::vec4>();
        case AttributeDataType::Invalid:
        default:
            throw std::logic_error("invalid AttributeDataType");
    }
}

template<typename F>
decltype(auto) attributeVecTypeHandler(const AttributeDataType dt, F&& f)
{
    switch (dt)
    {
        case AttributeDataType::Vec2:
            return f.template operator()<glm::vec2>();
        case AttributeDataType::Vec3:
            return f.template operator()<glm::vec3>();
        case AttributeDataType::Vec4:
            return f.template operator()<glm::vec4>();
        case AttributeDataType::Invalid:
        default:
            throw std::logic_error("invalid AttributeDataType");
    }
}

END_NAMESPACE