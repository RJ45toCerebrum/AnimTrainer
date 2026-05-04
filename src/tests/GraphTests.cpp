// Created by Tyler on 5/2/2026.

#include <numeric>
#include <gtest/gtest.h>
#include <print>
#include <random>

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

std::vector<float> generateRandomFloats(const size_t count)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution dis(0.0f, 10.0f);

    std::vector<float> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i)
        result.push_back(dis(gen));

    return result;
}

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

    const std::vector<float> rfs = generateRandomFloats(3);
    const float expectedResult = std::accumulate(rfs.begin(), rfs.end(), 0.0f);

    NodeHandle nh0 = graphRef.createNode(AddNode::kNodeTypeId, "Node0");
    EXPECT_TRUE(nh0.isValid());
    nh0.setUnpluggedInputByIndex(0, rfs[0]);
    nh0.setUnpluggedInputByIndex(1, rfs[1]);
    const auto outAttrOpt = nh0.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Output);
    EXPECT_TRUE(outAttrOpt.has_value());
    const AttrID outputAttrID = outAttrOpt.value();

    NodeHandle nh1 = graphRef.createNode(AddNode::kNodeTypeId, "Node1");
    EXPECT_TRUE(nh1.isValid());
    EXPECT_TRUE(nh1.setUnpluggedInputByIndex(1, rfs[2]));
    const auto inputAttrOpt = nh1.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Input);
    EXPECT_TRUE(inputAttrOpt.has_value());
    const AttrID inputAttrID = inputAttrOpt.value();

    EXPECT_TRUE(graphRef.connect(outputAttrID, inputAttrID));
    EXPECT_TRUE(graphRef.topologyChanged());

    const auto resultAttrIDOpt = nh1.fromNodeAttributeIndex(0, ATGraph::AttributeDirection::Output);
    EXPECT_TRUE(resultAttrIDOpt.has_value());
    const AttrID resultAttrID = resultAttrIDOpt.value();

    graphRef.evaluate();

    EXPECT_TRUE(not graphRef.topologyChanged());

    const std::span<const float> data = nh1.getData<float>(resultAttrID);
    EXPECT_TRUE(data.size() == 1);
    const float asbDiff = std::abs(data[0] - expectedResult);
    EXPECT_TRUE( asbDiff < std::numeric_limits<float>::epsilon() );
}

/* Next Test: Build Topo from JSON:
{
    "Node0" :
    {
        "type": "AddNode",
        "defaultInput": [3.0f, 7.0f],
        "connect" :
        [
            {
                "nodeName" : "Node1",
                "inputAttrIndex" : 0
            },
            {
                "nodeName" : "Node2",
                "inputAttrIndex" : 1
            }
        ]
    },
    "Node1" : ...,
    "Node2" : ...
}

 This will make testing substantially easier. Instead of manually connecting everything as in the above.
 */
