/**
 * This file is part of channeler.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#ifndef CHANNELER_VERSION_H
#define CHANNELER_VERSION_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <utility>
#include <string>

namespace channeler {

/**
 * XXX Note to developers (and users): consider the following definitions to be
 *     frozen. That is, you may add new definitions, or modify their values,
 *     but may not modify the definitions themselves (i.e. types, parameters).
 *
 *     That way users of this library can always rely, especially, on the
 *     version() function's prototype, and perform compatibility checks at
 *     runtime.
 **/

/**
 * Return the library version as a pair of two components.
 *
 * Depending on whether this build is a release build or not, the component are
 * either the branch name and subversion revision (development builds), or the
 * major and minor version numbers (release builds).
 **/
CHANNELER_API std::pair<std::string, std::string>
version();


/**
 * Return the library version as a string, with appropriate copyright notice.
 **/
CHANNELER_API char const *
copyright_string();


/**
 * Returns a short string with licensing information.
 **/
CHANNELER_API char const *
license_string();

} // namespace channeler

#endif // guard
