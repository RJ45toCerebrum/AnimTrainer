// Created by Tyler on 5/2/2026.

#include <gtest/gtest.h>
#include <print>

#include "Graph.h"
#include "Nodes/Math/AddNode.h"

using ATGraph::SceneGraph;
using ATGraph::NodeHandle;
using ATGraph::AddNode;
using ATGraph::AttrInfo;
using ATGraph::AttrID;
using ATGraph::AttributeDataType;


class GraphTestFixture : public ::testing::Test
{
public:
    std::optional<std::reference_wrapper<SceneGraph>> _graphInstance;

protected:
    void SetUp() override
    {
        std::println("Starting graph instance...");
        _graphInstance = SceneGraph::instance();
    }
    void TearDown() override
    {
        std::println("Stopping graph instance...");
        SceneGraph::destroy();
    }
};

TEST(ExampleSuite, ExampleTest)
{
    EXPECT_EQ(1 + 1, 2);
}

TEST_F(GraphTestFixture, CreateNodeTest)
{
    EXPECT_TRUE(_graphInstance.has_value());
    SceneGraph& graphRef = _graphInstance.value();
    ATGraph::NodePtr addNodePtr = std::make_unique<AddNode>();
    SceneGraph::registerNodeType(std::move(addNodePtr));
    NodeHandle newNodeHandle = graphRef.createNode(AddNode::kNodeTypeId, "NodeTest");
    EXPECT_TRUE(newNodeHandle.isValid());

    const std::vector<AttrInfo> inputAttrInfo = newNodeHandle.inputAttrInfo();
    const AttrInfo leftAttrInfo = inputAttrInfo[0];
    const AttrInfo rightAttrInfo = inputAttrInfo[1];

    EXPECT_TRUE(leftAttrInfo.type == AttributeDataType::Float);
    EXPECT_TRUE(rightAttrInfo.type == AttributeDataType::Float);

    const AttrID leftFloatAttr = leftAttrInfo.attrID;
    const AttrID rightFloatAttr = rightAttrInfo.attrID;
    EXPECT_TRUE(newNodeHandle.setUnpluggedInputAttrData<float>(leftFloatAttr, 1.0f));
    EXPECT_TRUE(newNodeHandle.setUnpluggedInputAttrData(rightFloatAttr, 2.0f));
}

TEST_F(GraphTestFixture, EdgeConnectsNodes)
{
    EXPECT_TRUE(_graphInstance.has_value());
    SceneGraph& graphRef = _graphInstance.value();
    ATGraph::NodePtr addNodePtr = std::make_unique<AddNode>();
    SceneGraph::registerNodeType(std::move(addNodePtr));

    NodeHandle nh0 = graphRef.createNode(AddNode::kNodeTypeId, "Node0");
    EXPECT_TRUE(nh0.isValid());
    nh0.setUnpluggedInputByIndex(0, 7.0f);

    NodeHandle nh1 = graphRef.createNode(AddNode::kNodeTypeId, "Node1");
    EXPECT_TRUE(nh1.isValid());
    nh1.setUnpluggedInputByIndex(1, 9.0f);


}
