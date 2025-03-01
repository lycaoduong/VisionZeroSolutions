// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/plugin.h>
#include <nx/sdk/analytics/i_engine.h>

namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            class Plugin: public nx::sdk::analytics::Plugin
            {
            protected:
                virtual nx::sdk::Result<nx::sdk::analytics::IEngine*> doObtainEngine() override;
                virtual std::string manifestString() const override;
            };

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx
