// Created by Tyler on 5/18/2026.

#include "Nodes/RegisterNodeTypes.h"
#include "Graph.h"
#include "Nodes/TimeNode.h"
#include "Nodes/Math/AddNode.h"
#include "Nodes/Math/MulOp.h"
#include "Nodes/Math/SinCosOp.h"
#include "Nodes/Math/VectorOp.h"

namespace ATGraph
{
    void registerBuiltInNodeTypes()
    {
        NodePtr timeNodePtr = std::make_unique<TimeNode>();
        SceneGraph::registerNodeType(std::move(timeNodePtr));
        NodePtr addNodePtr = std::make_unique<AddNode>();
        SceneGraph::registerNodeType(std::move(addNodePtr));
        NodePtr mulOp = std::make_unique<MulOp>();
        SceneGraph::registerNodeType(std::move(mulOp));
        NodePtr sinCosOp = std::make_unique<SinCosOp>();
        SceneGraph::registerNodeType(std::move(sinCosOp));
        NodePtr vectorOp = std::make_unique<VectorOp>();
        SceneGraph::registerNodeType(std::move(vectorOp));
    }
}