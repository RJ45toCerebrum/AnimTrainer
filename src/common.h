#pragma once

#define START_NAMESPACE(nsname) namespace nsname {
#define END_NAMESPACE }

// any code handling an observer ptr should never delete.
// You are NOT the owner of pointer...
template <typename T>
using Observer = T*;

