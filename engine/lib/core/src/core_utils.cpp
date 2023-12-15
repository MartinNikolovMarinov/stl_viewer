#include <core_utils.h>

namespace core {

namespace {
globalAssertHandlerPtr g_AssertHandler = nullptr;
} // namespace

void                   setGlobalAssertHandler(globalAssertHandlerPtr handler) { g_AssertHandler = handler; }
globalAssertHandlerPtr getGlobalAssertHandler()                               { return g_AssertHandler; }

} // namespace core
