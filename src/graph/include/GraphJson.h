// Created by Tyler on 5/4/2026.
#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

START_NAMESPACE(ATGraph)

class JsonGraphDataException : public std::runtime_error
{
public:
    explicit JsonGraphDataException(const std::string& msg) :
        std::runtime_error(msg)
    {}
};

struct JsonNodeConnectionData final
{
    std::string nodeName;
    uint16_t inputAttrIndex = 0;
    uint16_t outputAttrIndex = 0;
};

struct JsonNodeGraphData final
{
    std::string nodeName;
    std::string nodeTypeName;
    std::vector<JsonNodeConnectionData> connectionData;
};

/*
[
    {
        "nodeName": "NODE_NAME[0]",
        "nodeTypeName": "NODE_TYPE",
        // represents the connection from output attr to input attr of other node
        "connectionData" :
        [
          {
            "nodeName" : "NODE_NAME[1]",
            "outputAttrIndex": 0, // output attr index on this node (NODE_NAME[0])
            "inputAttrIndex" : 0 // input attr index on other node (NODE_NAME[1])
          }
        ]
    }
]
 */
std::vector<JsonNodeGraphData> parseGraphJSON(const std::string& json);

END_NAMESPACE