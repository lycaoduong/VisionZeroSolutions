// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "plugin.h"
#include "engine.h"


namespace nx {
    namespace analytics {
        namespace lcd_vision_solutions {

            using namespace nx::sdk;
            using namespace nx::sdk::analytics;

            Result<IEngine*> Plugin::doObtainEngine()
            {
                const auto utilityProvider = this->utilityProvider();
                const std::filesystem::path pluginHomeDir = utilityProvider->homeDir();
                return new Engine(pluginHomeDir);
                //return new Engine();
            }

            /**
             * JSON with the particular structure. Note that it is possible to fill in the values that are not
             * known at compile time.
             *
             * - id: Unique identifier for a plugin with format "{vendor_id}.{plugin_id}", where
             *     {vendor_id} is the unique identifier of the plugin creator (person or company name) and
             *     {plugin_id} is the unique (for a specific vendor) identifier of the plugin.
             * - name: A human-readable short name of the plugin (displayed in the "Camera Settings" window
             *     of the Client).
             * - description: Description of the plugin in a few sentences.
             * - version: Version of the plugin.
             * - vendor: Plugin creator (person or company) name.
             */
            std::string Plugin::manifestString() const
            {
                return /*suppress newline*/ 1 + (const char*) R"json(
                {
                    "id": "nx.clip.actions",
                    "name": "NX CLIP-Actions Plugin",
                    "description": ")json"
                        "This plugin is for CLIP-based Action Detection."
                        R"json(",
                    "version": "1.0.0",
                    "vendor": "LCD Vision Solutions."
                }
                )json";
            }

            /**
             * Called by the Server to instantiate the Plugin object.
             *
             * The Server requires the function to have C linkage, which leads to no C++ name mangling in the
             * export table of the plugin dynamic library, so that makes it possible to write plugins in any
             * language and compiler.
             *
             * NX_PLUGIN_API is the macro defined by CMake scripts for exporting the function.
             */
            extern "C" NX_PLUGIN_API nx::sdk::IPlugin* createNxPlugin()
            {
                // The object will be freed when the Server calls releaseRef().
                return new Plugin();
            }

        } // namespace lcd_vision_solutions
    } // namespace analytics
} // namespace nx
