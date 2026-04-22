// Created by Tyler on 4/21/2026.
#include "ATSceneGraph.h"

START_NAMESPACE(ATScene)


std::optional<std::reference_wrapper<ATSceneGraph>> getSceneGraph()
{
    throw std::exception("NO IMPL");
}

std::unique_ptr<ATSceneGraph> createATSceneGraph()
{
    throw std::exception("NO IMPL");
}


#pragma region ATTR_HANDLE
/// This is used as a bridge between AttributeHandle and Node containing attribute.
/// This avoids having to create another function in Scene graph for every method we want to call
/// on the scene node.
Observer<ATSceneNode> getSceneNode(const ATAttributeHandle& ah)
{
    std::optional<std::reference_wrapper<ATSceneGraph>> sgRef = getSceneGraph();
    if (not sgRef.has_value())
        return nullptr;

    const ATSceneGraph& sg = sgRef->get();
    // TODO: there should be some way to detect if the handle was used in a previous scene
    // For now I just skip this...
    const NodeID nodeID = ah.getNodeID();
    const auto itr = sg._nodes.find(nodeID);
    if (itr == sg._nodes.end())
        throw InvalidAttrHandle(ah);

    return itr->second.get();
}

bool ATAttributeHandle::isValid() const
{
    const std::optional<std::reference_wrapper<ATSceneGraph>> sgRef = getSceneGraph();
    if (not sgRef.has_value())
        return false;
    const ATSceneGraph& sg = sgRef->get();
    return sg.isValidAttrHandle(*this);
}

bool ATAttributeHandle::isDirty() const
{
    const Observer<ATSceneNode> sceneNode = getSceneNode(*this);
    assert(sceneNode != nullptr);
    assert(sceneNode->isValidAttrHandle(*this));
    return sceneNode->isDirty(*this);
}

uint32_t ATAttributeHandle::getElementCount() const
{
    const Observer<ATSceneNode> sceneNode = getSceneNode(*this);
    assert(sceneNode != nullptr);
    assert(sceneNode->isValidAttrHandle(*this));
    return sceneNode->getAttributeDataCount(*this);
}

AttributeDirection ATAttributeHandle::direction() const
{
    return _dataFlowDir;
}

AttributeDataType ATAttributeHandle::getDataType() const
{
    const Observer<ATSceneNode> sceneNode = getSceneNode(*this);
    assert(sceneNode != nullptr);
    assert(sceneNode->isValidAttrHandle(*this));
    return sceneNode->getAttrDataType(*this);
}

NodeID ATAttributeHandle::getNodeID() const
{
    return _nodeID;
}

std::string ATAttributeHandle::toString() const
{
    return std::format("[{}, {}, {}]",
        std::to_string(_nodeID), std::to_string(_index), attributeDirToStr(_dataFlowDir));
}

AttributeData ATAttributeHandle::getData() const
{
    const auto sgRef = getSceneGraph();
    if (not sgRef.has_value())
        return {nullptr, 0, AttributeDataType::Invalid};

    const ATSceneGraph& sceneGraph = sgRef.value();
    return sceneGraph.getData(*this);
}

/// It's an error to call this on attribute that has an input already because attr data is set by the graph...
/// Hence: `Unplugged Input Attr`
bool ATAttributeHandle::setUnpluggedInputAttrData(const AttributeData& inData) const
{
    if (_dataFlowDir != AttributeDirection::Input)
    {
        std::cerr << "[ERROR] Attempting to set the data of an attribute that is NOT an input attribute" << std::endl;
        return false;
    }
    const Observer<ATSceneNode> sceneNode = getSceneNode(*this);
    assert(sceneNode != nullptr);
    return sceneNode->setUnpluggedInputAttrData(*this, inData);
}
#pragma endregion


#pragma region ATTRIBUTE
bool ATAttribute::isDirty() const
{
    return _dirty;
}
bool ATAttribute::isPlugged() const
{
    // An attribute can only have a single input connection, but output attributes can have multiple outputs.
    if (_direction == AttributeDirection::Input)
    {
        const auto& handle = std::get<kInputAttrVariantIndex>(_sources);
        return handle.isValid();
    }
    const auto& handles = std::get<kOutputAttrVariantIndex>(_sources);
    return std::ranges::any_of(handles, [](const auto& handle) -> bool
    {
        return handle.isValid();
    });
}
bool ATAttribute::isInputAttribute() const
{
    return _direction == AttributeDirection::Input;
}
bool ATAttribute::isOutputAttribute() const
{
    return _direction == AttributeDirection::Output;
}
AttributeDataType ATAttribute::getDataType() const
{
    return _type;
}
bool ATAttribute::isArray() const
{
    return _array;
}
void ATAttribute::markDirty()
{
    _dirty = true;
}
void ATAttribute::markClean()
{
    // Should we check if upstream source is dirty?
    // might be too heavy. Maybe do all that checking in the Scene graph layer to keep this fast..
    _dirty = false;
}
#pragma endregion


#pragma region SCENE_NODE
bool ATSceneNode::isValidAttrHandle(const ATAttributeHandle& ah) const
{
    if (ah._nodeID != _nodeID)
        return false;

    if (ah._dataFlowDir != AttributeDirection::Input)
    {
        if (ah._index < 0 or ah._index >= _inputAttributes.size())
            return false;
    }
    else if (ah._index < 0 or ah._index >= _outputAttributes.size())
    {
        return false;
    }
    return true;
}
ATAttributeHandle ATSceneNode::convertToHandle(const AttributeDirection dataFlow, const uint16_t index) const
{
    return {_nodeID, index, dataFlow == AttributeDirection::Input ?
        AttributeDirection::Input : AttributeDirection::Output};
}
const ATSceneNode::AttributePtr& ATSceneNode::getAttributePtr(const ATAttributeHandle& ah) const
{
    if (not isValidAttrHandle(ah))
        throw InvalidAttrHandle(ah);

    if (ah._dataFlowDir == AttributeDirection::Input)
        return _inputAttributes[ah._index];

    return _outputAttributes[ah._index];
}
NodeID ATSceneNode::getNodeID() const
{
    return _nodeID;
}
const std::string& ATSceneNode::getName() const
{
    return _name;
}
void ATSceneNode::setName(const std::string& newName)
{
    _name = newName;
}
int32_t ATSceneNode::getAttributeDataCount(const ATAttributeHandle& ah) const
{
    if (not isValidAttrHandle(ah))
        return -1;

    if (ah._dataFlowDir == AttributeDirection::Input)
    {
        const AttributePtr& atrPtr = _inputAttributes[ah._index];
        return atrPtr->getDataCount();
    }
    const AttributePtr& atrPtr = _outputAttributes[ah._index];
    return atrPtr->getDataCount();
}
AttributeDataType ATSceneNode::getAttrDataType(const ATAttributeHandle& ah) const
{
    if (not isValidAttrHandle(ah))
        throw InvalidAttrHandle(ah);

    if (ah._dataFlowDir == AttributeDirection::Input)
    {
        const AttributePtr& attrPtr = _inputAttributes[ah._index];
        return attrPtr->getDataType();
    }
    const AttributePtr& attrPtr = _outputAttributes[ah._index];
    return attrPtr->getDataType();
}
int32_t ATSceneNode::getInputAttributeCount() const
{
    return _inputAttributes.size();
}
int32_t ATSceneNode::getOutputAttributeCount() const
{
    return _outputAttributes.size();
}
AttrHandleArray ATSceneNode::getInputAttrs() const
{
    AttrHandleArray inputAttrs;
    inputAttrs.reserve(_inputAttributes.size());
    for (int i = 0; i < _inputAttributes.size(); i++)
        inputAttrs.push_back(convertToHandle(AttributeDirection::Input, i));
    return inputAttrs;
}
AttrHandleArray ATSceneNode::getOutputAttrs() const
{
    AttrHandleArray outputAttrs;
    outputAttrs.reserve(_outputAttributes.size());
    for (int i = 0; i < _outputAttributes.size(); i++)
        outputAttrs.push_back(convertToHandle(AttributeDirection::Output, i));
    return outputAttrs;
}
AttrHandleArray ATSceneNode::getInputAttrs(const AttributeDataType attrDataType) const
{
    AttrHandleArray inputAttrs;
    for (int i = 0; i < _inputAttributes.size(); i++)
    {
        const auto& attrPtr = _inputAttributes[i];
        if (attrPtr->getDataType() == attrDataType)
            inputAttrs.push_back(convertToHandle(AttributeDirection::Input, i));
    }
    return inputAttrs;
}
AttrHandleArray ATSceneNode::getOutputAttrs(const AttributeDataType attrDataType) const
{
    AttrHandleArray outputAttrs;
    for (int i = 0; i < _outputAttributes.size(); i++)
    {
        const auto& attrPtr = _outputAttributes[i];
        if (attrPtr->getDataType() == attrDataType)
            outputAttrs.push_back(convertToHandle(AttributeDirection::Output, i));
    }
    return outputAttrs;
}
bool ATSceneNode::isDirty(const ATAttributeHandle& ah) const
{
    if (not isValidAttrHandle(ah))
        throw InvalidAttrHandle(ah);

    if (ah._dataFlowDir == AttributeDirection::Input)
    {
        const AttributePtr& attrPtr = _inputAttributes[ah._index];
        return attrPtr->isDirty();
    }
    const AttributePtr& attrPtr = _outputAttributes[ah._index];
    return attrPtr->isDirty();
}
bool ATSceneNode::isPlugged(const ATAttributeHandle& ah) const
{
    const auto& ptr = getAttributePtr(ah);
    return ptr->isPlugged();
}
bool ATSceneNode::setUnpluggedInputAttrData(const ATAttributeHandle& ah, const AttributeData& attrData) const
{
    const auto& ptr = getAttributePtr(ah);
    if (ptr->isPlugged())
    {
        std::cerr << "[ATSceneNode::setUnpluggedInputAttrData] "
                     "Attempting to set data on attribute that already has input" << std::endl;
        return false;
    }
    return ptr->setData(attrData);
}


#pragma endregion

END_NAMESPACE