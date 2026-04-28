// Created by Tyler on 4/20/2026.
#pragma once

#include "common.h"

#include <exception>
#include <span>
#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <print>
#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <ATMath.h>


START_NAMESPACE(ATScene)

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;
using glm::mat4x4;

// this is defined here rather than in ATSceneNode.h to avoid circular dependency
using NodeID = uint32_t;
constexpr NodeID kInvalidNodeID = 0;
using AttrHandleArray = std::vector<class ATAttributeHandle>;

enum class AttributeDataType : uint16_t
{
    Invalid,
    Bool,
    Int,
    Float,
    String,
    Vec2,
    Vec3,
    Vec4,
    Quaternion,
    Mat4x4,
    Transform,
    Mesh,
    Custom
};

// compile-time typename T => AttributeDataType
template<typename T>
consteval AttributeDataType getEnumForType()
{
    if constexpr (std::is_same_v<T, bool>)
        return AttributeDataType::Bool;
    else if constexpr (std::is_same_v<T, int>)
        return AttributeDataType::Int;
    else if constexpr (std::is_same_v<T, float>)
        return AttributeDataType::Float;
    else if constexpr (std::is_same_v<T, std::string>)
        return AttributeDataType::String;
    else if constexpr (std::is_same_v<T, glm::vec2>)
        return AttributeDataType::Vec2;
    else if constexpr (std::is_same_v<T, glm::vec3>)
        return AttributeDataType::Vec3;
    else if constexpr (std::is_same_v<T, glm::vec4>)
        return AttributeDataType::Vec4;
    else if constexpr (std::is_same_v<T, glm::quat>)
        return AttributeDataType::Quaternion;
    else if constexpr (std::is_same_v<T, mat4x4>)
        return AttributeDataType::Mat4x4;
    else if constexpr (std::is_same_v<T, ATMath::Transform>)
        return AttributeDataType::Transform;

    throw std::exception("Type T not implemented yet");
}


template<typename T>
concept BasicAttributeType =
    std::same_as<T, bool> || std::same_as<T, int> ||
    std::same_as<T, float> || std::same_as<T, std::string> ||
    std::same_as<T, glm::vec2> || std::same_as<T, glm::vec3> ||
    std::same_as<T, glm::vec4> || std::same_as<T, glm::quat> ||
    std::same_as<T, glm::mat4x4> || std::same_as<T, ATMath::Transform>;

enum class AttributeDirection : uint8_t
{
    Input,
    Output
};

inline std::string_view attributeDirToStr(const AttributeDirection dir)
{
    static std::string inputStr = "input";
    static std::string outputStr = "output";
    if (dir == AttributeDirection::Input)
        return inputStr;
    return outputStr;
}

// DO NOT store these
class AttributeData final
{
    const void* _rawData = nullptr;
    const int _elementCount = 0;
    const AttributeDataType _dataType;

public:
    // TODO: provide constructor types for all the types
    AttributeData(const void* rawData, const int elementCount, const AttributeDataType dataType) :
        _rawData(rawData), _elementCount(elementCount), _dataType(dataType)
    {
        assert(rawData != nullptr);
        assert(elementCount >= 1);
    }

    [[nodiscard]] bool isValid() const
    {
        return _rawData != nullptr and _elementCount >= 1;
    }
    [[nodiscard]] AttributeDataType getDataType() const
    {
        return _dataType;
    }
    // for controlled casting.
    template<typename T>
    const T& getData() const
    {
        assert(_rawData != nullptr);
        assert(_elementCount == 1);
        if (_dataType != getEnumForType<T>())
            throw std::runtime_error("Template type T does not match internal dataType");
        return *static_cast<const T*>(_rawData);
    }

    template<typename T>
    std::span<const T> getDataArray() const
    {
        assert(_rawData != nullptr);
        assert(_elementCount > 1);
        if (_dataType != getEnumForType<T>())
            throw std::runtime_error("Template type T does not match internal dataType");
        const T* ptr = static_cast<const T*>(_rawData);
        return std::span<const T>(ptr, _elementCount);
    }
};

class ATAttributeHandle final
{
    friend class ATSceneNode;

    /// TODO: probably should have a generation hash to ensure this handle is valid for current scene graph (aka scene reload)
    NodeID _nodeID;
    uint16_t _index;
    AttributeDirection _dataFlowDir;

private:
    // ONLY the scene node should create attribute handles because its only entity that has sufficient information to create it.
    ATAttributeHandle(const NodeID inNodeID, const uint16_t inIndex, const AttributeDirection inDir) :
        _nodeID(inNodeID), _index(inIndex), _dataFlowDir(inDir)
    {}

public:
    // we allow construction of default handle which is the invalid handle
    ATAttributeHandle() :
        _nodeID(ATScene::kInvalidNodeID), _index(0), _dataFlowDir()
    {}
    ATAttributeHandle(const ATAttributeHandle& handle) = default;
    ATAttributeHandle& operator=(const ATAttributeHandle& handle) = default;
    bool operator==(const ATAttributeHandle& handle) const
    {
        return _nodeID == handle._nodeID and _index == handle._index and _dataFlowDir == handle._dataFlowDir;
    }
    bool operator!=(const ATAttributeHandle& handle) const
    {
        return !(*this == handle);
    }

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isDirty() const;
    [[nodiscard]] bool areCompatible(ATAttributeHandle otherHandle) const;
    [[nodiscard]] uint32_t getElementCount() const;
    [[nodiscard]] AttributeDirection direction() const;
    [[nodiscard]] AttributeDataType getDataType() const;
    [[nodiscard]] NodeID getNodeID() const;
    [[nodiscard]] std::string toString() const;


    [[nodiscard]] AttributeData getData();
    /// It's an error to call this on attribute that has an input already because attr data is set by the graph...
    /// Hence: `Unplugged Input Attr`
    [[nodiscard]] bool setUnpluggedInputAttrData(const AttributeData& inData) const;
};

class InvalidAttrHandle final : public std::runtime_error
{
    const ATAttributeHandle _handle;
public:
    explicit InvalidAttrHandle(const ATAttributeHandle handle) :
        std::runtime_error(formatMessage(handle)), _handle(handle)
    {}
    [[nodiscard]] std::string getHandleString() const
    {
        return _handle.toString();
    }
    static std::string formatMessage(const ATAttributeHandle handle)
    {
        return "Invalid Attribute Handle: " + handle.toString() + "\n" +
               "Most common reason is node was destroyed (explicitly or implicitly (via scene reload)).\n";
    }
};

class ATAttribute
{
    friend class ATSceneNode;

    static constexpr uint8_t kInputAttrVariantIndex = 0;
    static constexpr uint8_t kOutputAttrVariantIndex = 1;

    const bool _array;
    const AttributeDataType _type;
    const AttributeDirection _direction;
    const NodeID _owner;
    // Important NOTE:
    // Either this is an Input attribute or an output attribute (hence the variant).
    // We do not allow attributes to be both input and output attributes.
    // IF input is "plugged in" by an output attribute of a different node THEN:
    // data does not directly live in the attribute pointed to by this handle. Rather,
    // The data solely lives in the output attribute. This avoids copy data as it flows through the graph...
    // Getting data from its upstream attr (the output attribute).
    // DO NOT switch the orientation of types (_sources) here as index is checked in getDataType
    bool _dirty = false;
    std::variant<ATAttributeHandle, std::vector<ATAttributeHandle>> _sources;

public:
    // TODO: make construction protected...
    // scene nodes are solely responsible for construction of attributes
    ATAttribute(const AttributeDataType type, const NodeID owner, const bool isArray, const AttributeDirection direction) :
        _array(isArray), _type(type), _direction(direction), _owner(owner)
    {
        if (_direction == AttributeDirection::Input)
            _sources.emplace<ATAttributeHandle>();
        else
            _sources.emplace<std::vector<ATAttributeHandle>>();
    }
    ATAttribute() = delete;
    ATAttribute(const ATAttribute&) = delete;
    ATAttribute(ATAttribute&&) = delete;
    ATAttribute& operator=(const ATAttribute&) = delete;
    virtual ~ATAttribute() = default;

    [[nodiscard]] bool isDirty() const;
    [[nodiscard]] bool isPlugged() const;
    [[nodiscard]] bool isInputAttribute() const;
    [[nodiscard]] bool isOutputAttribute() const;
    [[nodiscard]] AttributeDataType getDataType() const;
    [[nodiscard]] bool isArray() const;

protected:
    // NOTE: the plug methods do not check if a cycle will be formed. This should be checked by graph
    bool plugIncoming(ATAttributeHandle outputAttr);
    bool plugOutgoing(ATAttributeHandle inputAttr);
    bool unplug(ATAttributeHandle ah);

    [[nodiscard]] virtual int32_t getDataCount() const = 0;
    [[nodiscard]] virtual AttributeData getRawData() const = 0;
    virtual bool setData(const AttributeData& attrData) = 0;

private:
    // this only sets the _dirty flag. SceneGraph is responsible for marking all downstream attributes...
    void markDirty();
    void markClean();
    [[nodiscard]] std::vector<ATAttributeHandle> getSources() const;
};

END_NAMESPACE