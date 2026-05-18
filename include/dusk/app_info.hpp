#ifndef DUSK_APPNAME_HPP
#define DUSK_APPNAME_HPP

namespace dusk {
    /**
     * \brief The internal application name for the game.
     *
     * This gets used for file paths and such, and cannot be changed!
     */
    constexpr auto AppName = "Dawnlight";

    /**
     * Previous AppName to migrate data from.
     */
    constexpr auto LegacyAppName = "Dusk";

    /**
     * \brief The internal organization name for the game.
     *
     * This gets used for file paths and such, and cannot be changed!
     */
    constexpr auto OrgName = "BeZide93";
}

#endif  // DUSK_APPNAME_HPP
