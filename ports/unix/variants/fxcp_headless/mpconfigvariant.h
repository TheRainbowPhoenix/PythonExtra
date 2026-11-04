// Linux counterpart of the Windows fxcp_headless variant.
// Keep the Unix feature set while sharing the same gint tracing API.
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)
#include "../mpconfigvariant_common.h"
#define PE_HEADLESS_TIME (1)
