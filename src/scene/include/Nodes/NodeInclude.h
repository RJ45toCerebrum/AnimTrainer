// Created by Tyler on 4/28/2026.

// Where we place all node headers for easier include of all the nodes
#pragma once

#include "Nodes/ATDebugSphere.h"
#include "Nodes/TimeNode.h"

START_NAMESPACE(ATNode)

inline void registerNodeTypes()
{
    TimeNodeFactory::registerNodeFactory();
    DebugSphereNodeFactory::registerNodeFactory();
}

END_NAMESPACE
