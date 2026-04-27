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

/// This is used as a bridge between AttributeHandle and Node containing attribute.
/// This avoids having to create another function in Scene graph for every method we want to call
/// on the scene node. We do not want this callable outside.
Observer<ATSceneNode> getSceneNode(const ATAttributeHandle ah)
{
    const auto sgRef = getSceneGraph();
    if (not sgRef.has_value())
        return nullptr;

    const ATSceneGraph& sg = sgRef->get();
    // TODO: there should be some way to detect if the handle was used in a previous scene. For now I just skip that...
    const NodeID nodeID = ah.getNodeID();
    if (not sg.isValidNodeID(nodeID))
        throw InvalidAttrHandle(ah);

    return sg._nodes[nodeID].get();
}


#pragma region ATTR_HANDLE
bool ATAttributeHandle::isValid() const
{
    const auto sgRef = getSceneGraph();
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

bool ATAttributeHandle::areCompatible(const ATAttributeHandle otherHandle) const
{
    return getDataType() == otherHandle.getDataType();
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

AttributeData ATAttributeHandle::getData()
{
    const auto sgRef = getSceneGraph();
    if (not sgRef.has_value())
        return {nullptr, 0, AttributeDataType::Invalid};

    ATSceneGraph& sceneGraph = sgRef.value();
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
    // scene node responsible for marking downstream
    _dirty = true;
}

void ATAttribute::markClean()
{
    _dirty = false;
}

std::vector<ATAttributeHandle> ATAttribute::getSources() const
{
    if (isInputAttribute())
        return { std::get<kInputAttrVariantIndex>(_sources) };

    return std::get<kOutputAttrVariantIndex>(_sources);
}

bool ATAttribute::plugIncoming(const ATAttributeHandle outputAttr)
{
    // Here we just straight set it without checking for cycles.
    // This is the job of the scene graph to check for cycles before this is called...
    if(not isInputAttribute())
    {
        std::cerr << "[ATAttribute::plugIncoming] "
                     "call to plugIncoming on attribute that is output attribute" << std::endl;
        return false;
    }
    // we do not allow implicit unplug. IF feel like that would produce lots of bugs...
    if(isPlugged())
    {
        std::cerr << "[ATAttribute::plugIncoming] "
                     "call to plugIncoming on input attribute but the attribute is already plugged" << std::endl;
        return false;
    }
    if (not outputAttr.isValid())
    {
        std::cerr << "[ATAttribute::plugIncoming] "
             "Can not plug with invalid output attribute..." << std::endl;
        return false;
    }
    // need to check if the attributes are compatible (aka same data types)
    if (_type != outputAttr.getDataType())
    {
        std::cerr << "[ATAttribute::plugIncoming] "
             "call to plugIncoming on input attribute but the data types are not compatible" << std::endl;
        return false;
    }
    _sources.emplace<kInputAttrVariantIndex>(outputAttr);
    return true;
}

bool ATAttribute::plugOutgoing(const ATAttributeHandle inputAttr)
{
    if (not isOutputAttribute())
    {
        std::cerr << "[ATAttribute::plugOutgoing] "
             "call to plugOutgoing on attribute that is an input attribute" << std::endl;
        return false;
    }
    if (not inputAttr.isValid())
    {
        std::cerr << "[ATAttribute::plugOutgoing] "
            "Can not plug with invalid input attribute..." << std::endl;
        return false;
    }
    // need to check if the attributes are compatible (aka same data types)
    if (_type != inputAttr.getDataType())
    {
        std::cerr << "[ATAttribute::plugOutgoing] "
             "call to plugOutgoing on input attribute but the data types are not compatible" << std::endl;
        return false;
    }

    auto& outputSources = std::get<kOutputAttrVariantIndex>(_sources);
    outputSources.push_back(inputAttr);
    return true;
}

bool ATAttribute::unplug(const ATAttributeHandle ah)
{
    if (isInputAttribute())
    {
        const ATAttributeHandle& inputSource = std::get<kInputAttrVariantIndex>(_sources);
        if (inputSource == ah)
        {
            _sources.emplace<kInputAttrVariantIndex>(ATAttributeHandle());
            return true;
        }
        return false;
    }

    auto& outputSources = std::get<kOutputAttrVariantIndex>(_sources);
    const auto fitr = std::ranges::find(outputSources, ah);
    outputSources.erase(fitr);
    return true;
}
#pragma endregion




#pragma region SCENE_NODE
bool ATSceneNode::isValidAttrHandle(const ATAttributeHandle ah) const
{
    if (ah._nodeID != _nodeID or ah._nodeID == kInvalidNodeID)
        return false;

    if (ah._dataFlowDir == AttributeDirection::Input)
    {
        if (ah._index >= _inputAttributes.size())
            return false;
    }
    else if (ah._index >= _outputAttributes.size())
    {
        return false;
    }
    return true;
}

ATAttributeHandle ATSceneNode::convertToHandle(const AttributeDirection dataFlow, const uint16_t index) const
{
    return {_nodeID, index, dataFlow};
}

ATAttribute& ATSceneNode::getAttributeRef(const ATAttributeHandle ah) const
{
    if (not isValidAttrHandle(ah))
        throw InvalidAttrHandle(ah);

    if (ah._dataFlowDir == AttributeDirection::Input)
        return *_inputAttributes[ah._index];

    return *_outputAttributes[ah._index];
}

ATAttribute& ATSceneNode::getAttributeRefUnchecked(const ATAttributeHandle ah) const
{
    if (ah._dataFlowDir == AttributeDirection::Input)
        return *_inputAttributes[ah._index];
    return *_outputAttributes[ah._index];
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

int32_t ATSceneNode::getAttributeDataCount(const ATAttributeHandle ah) const
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

AttributeDataType ATSceneNode::getAttrDataType(const ATAttributeHandle ah) const
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

AttrHandleArray ATSceneNode::getDirtyInputAttrs() const
{
    AttrHandleArray inputAttrs;
    for (int i = 0; i < _inputAttributes.size(); i++)
    {
        const ATAttribute& attr = *_inputAttributes[i];
        if (attr.isDirty())
            inputAttrs.push_back(convertToHandle(AttributeDirection::Input, i));
    }
    return inputAttrs;
}

bool ATSceneNode::isDirty(const ATAttributeHandle ah) const
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

bool ATSceneNode::isPlugged(const ATAttributeHandle ah) const
{
    const auto& ptr = getAttributeRef(ah);
    return ptr.isPlugged();
}

bool ATSceneNode::setUnpluggedInputAttrData(const ATAttributeHandle ah, const AttributeData attrData)
{
    if (not isValidAttrHandle(ah))
        throw InvalidAttrHandle(ah);

    auto& attrRef = *_inputAttributes.at(ah._index);
    if (attrRef.isPlugged())
    {
        std::cerr << "[ATSceneNode::setUnpluggedInputAttrData] "
                     "Attempting to set data on attribute that already has input" << std::endl;
        return false;
    }
    if (not attrRef.setData(attrData))
        return false;

    markInputDirty(ah);
    return true;
}

// NOTE: this method does not check for cycle. Graph is responsible for checking that before calling this.
// Also note that this method is one directional. When graph connects attr's it needs to call symetrically.
bool ATSceneNode::plugAttribute(const ATAttributeHandle thisNodeAttribute, const ATAttributeHandle otherNodeAttribute)
{
    ATAttribute& attrRef = getAttributeRef(thisNodeAttribute);
    if (attrRef.isInputAttribute())
        return attrRef.plugIncoming(otherNodeAttribute);

    return attrRef.plugOutgoing(otherNodeAttribute);
}

bool ATSceneNode::unplugAttribute(const ATAttributeHandle ah)
{
    ATAttribute& attrRef = getAttributeRef(ah);
    if (not attrRef.isPlugged())
    {
        std::cout << "[ATSceneNode::unplugAttribute] attribute not plugged" << std::endl;
        return false;
    }
    std::vector<ATAttributeHandle> sources = attrRef.getSources();
    if (attrRef.isInputAttribute())
    {
        // IF ah refers to input attribute on this node THEN there can only be 1 source.
        assert(sources.size() == 1);
        return attrRef.unplug(sources[0]);
    }
    bool success = true;
    for (const auto source : sources)
    {
        if (not attrRef.unplug(source))
        {
            std::cerr << "[ATSceneNode::unplugAttribute] Failed to unplug" << std::endl;
            success = false;
        }
    }
    return success;
}

void ATSceneNode::markInputDirty(const ATAttributeHandle ah)
{
    auto& attrRef = *_inputAttributes.at(ah._index);
    attrRef.markDirty();
    // For now, to keep things simple, marking any input dirty automatically marks all outputs dirty
    // which triggers all downstream attributes to become dirty
    markOutputsDirty();
}

void ATSceneNode::markInputClean(const ATAttributeHandle ah)
{
    assert(ah.direction() == AttributeDirection::Input);
    if (not isValidAttrHandle(ah))
        throw InvalidAttrHandle(ah);

    ATAttribute& attr = *_inputAttributes[ah._index];
    attr.markClean();
}

void ATSceneNode::markOutputClean(const ATAttributeHandle ah)
{
    // similar to markOutputsDirty, we need to mark downstream clean.
    // unlike markOutputsDirty it does not recurse because compute must be called before downstream clean.
    assert(ah.direction() == AttributeDirection::Output);
    if (not isValidAttrHandle(ah))
        throw InvalidAttrHandle(ah);

    ATAttribute& attr = *_outputAttributes[ah._index];
    attr.markClean();
    for (const ATAttributeHandle source : attr.getSources())
    {
        Observer<ATSceneNode> node = getSceneNode(source);
        assert(node != nullptr);
        node->markInputClean(source);
    }
}

// outputs are responsible for marking input attr dirty.
// DO NOT mark const. Yes, ATSceneNode itself does not have changing state BUT the attributes it solely owns
// are changing. Logically its non-const.
void ATSceneNode::markOutputsDirty()
{
    // although this looks like bad time complexity (which it is under many nodes each having many downstream connections);
    // This is what makes the graph fast. We only mark the downstream dirty (no compute).
    // We do not compute entire graph every frame. We only compute the dirty nodes AND only if data requested.
    for (auto& outputAttrPtr : _outputAttributes)
    {
        // do not recurse if we already marked this dirty as all downstream is already dirty
        if (outputAttrPtr->isDirty())
            continue;
        outputAttrPtr->markDirty();
        for (const auto& source : outputAttrPtr->getSources())
        {
            Observer<ATSceneNode> downstreamNode = getSceneNode(source);
            assert(downstreamNode != nullptr);
            downstreamNode->markInputDirty(source);
        }
    }
}

ATAttributeHandle ATSceneNode::registerInputAttribute(AttributePtr newAttribute)
{
    _inputAttributes.push_back(std::move(newAttribute));
    assert(_inputAttributes.size() < std::numeric_limits<uint16_t>::max());
    const uint16_t attrID = _inputAttributes.size() - 1;
    return {_nodeID, attrID, AttributeDirection::Input};
}
ATAttributeHandle ATSceneNode::registerOutputAttribute(AttributePtr newAttribute)
{
    _outputAttributes.push_back(std::move(newAttribute));
    assert(_outputAttributes.size() < std::numeric_limits<uint16_t>::max());
    const uint16_t attrID = _outputAttributes.size() - 1;
    return {_nodeID, attrID, AttributeDirection::Output};
}

AttributeData ATSceneNode::getAttributeData(const ATAttributeHandle ah) const
{
    if (isDirty(ah))
        throw std::runtime_error("[ATSceneNode::getAttributeData] Should only be called after"
                                 " scene graph has fully recomputed upstream dependents.");

    const ATAttribute& attr = getAttributeRefUnchecked(ah);
    return attr.getRawData();
}

ATAttributeHandle ATSceneNode::getUpstreamHandle(const ATAttributeHandle inputAttribute) const
{
    const ATAttribute& attrRef = getAttributeRef(inputAttribute);
    assert(attrRef.isInputAttribute());
    const auto sources = attrRef.getSources();
    return sources.at(0);
}
#pragma endregion




#pragma region SCENE_GRAPH
ATSceneGraph::ATSceneGraph()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    _sceneHash = dis(gen);

    // TODO: polymorphic memory pool
    _nodes.reserve(50);
}

ATSceneGraph::~ATSceneGraph()
{
    _nodes.clear();
}

NodeID ATSceneGraph::createNode(const NodeTypeID typeId, const std::string& name)
{
    const auto fItr = _factories.find(typeId);
    if (fItr == _factories.end())
        throw NodeFactoryNotFound(typeId);

    ISceneNodeFactory& factory = *fItr->second;
    const NodeID newNodeID = _nodes.size();
    SceneNodePtr newNode = factory.createSceneNode(newNodeID, name);
    assert(newNode != nullptr);

    _nodes.push_back(std::move(newNode));
    return newNodeID;
}

bool ATSceneGraph::deleteNode(const NodeID nodeId)
{
    if (not isValidNodeID(nodeId))
        throw std::runtime_error("[ATSceneGraph::deleteNode] Invalid node ID");

    throw std::runtime_error("[ATSceneGraph::deleteNode] NO IMPL");
}

AttrHandleArray ATSceneGraph::getNodeOutputHandles(const NodeID nodeId) const
{
    if (not isValidNodeID(nodeId))
        throw std::runtime_error("[ATSceneGraph::getNodeOutputHandles] Invalid node ID");

    const ATSceneNode& nodeRef = *_nodes[nodeId];
    return nodeRef.getOutputAttrs();
}

AttrHandleArray ATSceneGraph::getNodeInputHandles(const NodeID nodeId) const
{
    if (not isValidNodeID(nodeId))
        throw std::runtime_error("[ATSceneGraph::getNodeInputHandles] Invalid node ID");

    const ATSceneNode& nodeRef = *_nodes[nodeId];
    return nodeRef.getOutputAttrs();
}

bool ATSceneGraph::isValidAttrHandle(const ATAttributeHandle attrHandle) const
{
    if (not isValidNodeID(attrHandle.getNodeID()))
        return false;

    const ATSceneNode& node = *_nodes[attrHandle.getNodeID()];
    return node.isValidAttrHandle(attrHandle);
}

bool ATSceneGraph::isDirtyAttr(const ATAttributeHandle attrHandle) const
{
    if (not isValidNodeID(attrHandle.getNodeID()))
        throw std::runtime_error("[ATSceneGraph::isDirtyAttr] Node ID is out of range");

    const ATSceneNode& node = *_nodes[attrHandle.getNodeID()];
    return node.isDirty(attrHandle);
}

std::expected<bool,std::string> ATSceneGraph::isInputAttrPlugged(const ATAttributeHandle attrHandle) const
{
    const NodeID nodeID = attrHandle.getNodeID();
    if (not isValidNodeID(nodeID))
        return std::unexpected("Invalid input attribute. Was the node destroyed?");

    const ATSceneNode& node = *_nodes[nodeID];
    return node.isPlugged(attrHandle);
}

std::expected<AttributeData,int> ATSceneGraph::getData(const ATAttributeHandle attrHandle)
{
    const NodeID nodeID = attrHandle.getNodeID();
    // TODO: make proper return enum.
    if (not isValidNodeID(nodeID))
        return std::unexpected(1);
    if (attrHandle.direction() == AttributeDirection::Input)
        return std::unexpected(2);

    evaluateGraph(attrHandle);

    ATSceneNode& node = *_nodes[nodeID];
    return node.getAttributeData(attrHandle);
}

void ATSceneGraph::evaluateGraph(const ATAttributeHandle attrHandle)
{
    const NodeID nodeID = attrHandle.getNodeID();
    if (not isValidNodeID(nodeID) or attrHandle.direction() == AttributeDirection::Input)
        return;

    ATSceneNode& node = *_nodes[nodeID];
    // are inputs already clean? no work to do on graph
    if (not node.isDirty(attrHandle))
        return;

    // in order to compute this node, we need to go upstream
    const std::vector<ATAttributeHandle> dirtyHandles = node.getDirtyInputAttrs();
    for (const auto dirtyHandle : dirtyHandles)
    {
        if (not node.isPlugged(dirtyHandle))
        {
            // for non-plugged input attributes, we just mark clean.
            node.markInputClean(dirtyHandle);
            continue;
        }
        // get the upstream handle
        const ATAttributeHandle upstreamHandle = node.getUpstreamHandle(dirtyHandle);
        evaluateGraph(upstreamHandle);
    }
    node.compute(attrHandle);
}

//bool ATSceneGraph::willFormCycle(ATAttributeHandle outputHandle, ATAttributeHandle inputHandle) const;
//bool ATSceneGraph::connect(ATAttributeHandle outputHandle, ATAttributeHandle inputHandle);
//bool ATSceneGraph::disconnect(ATAttributeHandle inputHandle);
#pragma endregion

END_NAMESPACE