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
#ifndef CHANNELER_MACROS_H
#define CHANNELER_MACROS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <liberate/logging.h>

// error_t logging
#define LIBLOG_ET(msg, code) LIBLOG_ERROR(msg << " // " << error_name(code) << ": " \
    << error_message(code));


/**
 * Flow control guard; if this line is reached, an exception is thrown.
 **/
#define CHANNELER_FLOW_CONTROL_GUARD_WITH(msg)                      \
  {                                                                 \
    std::stringstream flowcontrol;                                  \
    std::string mess{msg};                                          \
    if (!mess.empty()) {                                            \
      flowcontrol << mess << " - ";                                 \
    }                                                               \
    flowcontrol << "Control should never have reached this line: "  \
      << __FILE__ << ":" << __LINE__;                               \
    throw exception(ERR_UNEXPECTED, flowcontrol.str());             \
  }

#define CHANNELER_FLOW_CONTROL_GUARD CHANNELER_FLOW_CONTROL_GUARD_WITH("")


/**
 * Support for clang/OCLint suppressions
 **/
#if defined(__clang__) and defined(OCLINT_IS_RUNNING)
#  define OCLINT_SUPPRESS(suppression) \
    __attribute__(( \
      annotate("oclint:suppress[" suppression "]") \
    ))
#else
#  define OCLINT_SUPPRESS(annotation)
#endif

#endif // guard
