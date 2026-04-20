#pragma once

#define START_NAMESPACE(nsname) namespace nsname {
#define END_NAMESPACE }

// any code handling an observer ptr should never delete.
// this is in case we need to pass raw pointers around rather than shared/unique/weak
template <typename T>
using observer = T*;

