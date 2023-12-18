#pragma once

#include <stlv.h>

NO_MANGLE bool create(stlv::AppCreateInfo* createInfo);
NO_MANGLE void shutdown();
NO_MANGLE bool update(stlv::ApplicationState* appState);
