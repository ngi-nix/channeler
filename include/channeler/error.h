/**
 * This file is part of channeler.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2021 Jens Finkhaeuser.
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
#ifndef CHANNELER_ERROR_H
#define CHANNELER_ERROR_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <string>
#include <stdexcept>

/**
 * Macros for building a table of known error codes and associated messages.
 *
 * XXX Note that the macros contain namespace definitions, but are not expanded
 *     in a namespace scope. This is to allow files that include this header to
 *     decide what namespace/scope macros are to be expanded in.
 **/
#if !defined(CHANNELER_START_ERRORS)
#define CHANNELER_START_ERRORS \
  namespace channeler { \
  using error_t = uint32_t; \
  enum CHANNELER_API : error_t {
#define CHANNELER_ERRDEF(name, code, desc) name = code,
#define CHANNELER_END_ERRORS \
    CHANNELER_ERROR_LAST, \
    CHANNELER_START_USER_RANGE = 1000 }; }
#define CHANNELER_ERROR_FUNCTIONS
#endif


/*****************************************************************************
 * Error definitions
 **/
CHANNELER_START_ERRORS

CHANNELER_ERRDEF(ERR_SUCCESS,
    0,
    "No error")

CHANNELER_ERRDEF(ERR_UNEXPECTED,
    1,
    "Nobody expects the Spanish Inquisition!")

CHANNELER_ERRDEF(ERR_INSUFFICIENT_BUFFER_SIZE,
    2,
    "The provided buffer is too small for the data type!")

CHANNELER_ERRDEF(ERR_DECODE,
    3,
    "Could not decode data buffer.")

CHANNELER_ERRDEF(ERR_ENCODE,
    4,
    "Could not encode data buffer.")

CHANNELER_ERRDEF(ERR_INVALID_CHANNELID,
    5,
    "Channel identifier is invalid.")

CHANNELER_ERRDEF(ERR_INVALID_REFERENCE,
    6,
    "A reference does not resolve to a valid object.")

CHANNELER_ERRDEF(ERR_INVALID_PIPE_EVENT,
    7,
    "An filter could not handle the event type passed to it.")

CHANNELER_ERRDEF(ERR_INVALID_MESSAGE_TYPE,
    8,
    "Unknown message type was received.")

CHANNELER_ERRDEF(ERR_WRITE,
    9,
    "Write error.")

CHANNELER_ERRDEF(ERR_STATE,
    10,
    "State machine error.")


CHANNELER_END_ERRORS


#if defined(CHANNELER_ERROR_FUNCTIONS)

namespace channeler {

/*****************************************************************************
 * Functions
 **/

/**
 * Return the error message associated with the given error code. Never returns
 * nullptr; if an unknown error code is given, an "unidentified error" string is
 * returned. Not that this should happen, given that error_t is an enum...
 **/
CHANNELER_API char const * error_message(error_t code);

/**
 * Return a string representation of the given error code. Also never returns
 * nullptr, see error_message() above.
 **/
CHANNELER_API char const * error_name(error_t code);



/*****************************************************************************
 * Exception
 **/

/**
 * Exception class. Constructed with an error code and optional message;
 * wraps error_message() and error_name() above.
 **/
class CHANNELER_API exception : public std::runtime_error
{
public:
  /**
   * Constructor/destructor
   **/
  explicit exception(error_t code, std::string const & details = std::string()) throw();
  virtual ~exception() throw();

  /**
   * std::exception interface
   **/
  virtual char const * what() const throw();

  /**
   * Additional functions
   **/
  char const * name() const throw();
  error_t code() const throw();

private:
  error_t     m_code;
  std::string m_message;
};


} // namespace channeler

#endif // CHANNELER_ERROR_FUNCTIONS


/**
 * Undefine macros again
 **/
#undef CHANNELER_START_ERRORS
#undef CHANNELER_ERRDEF
#undef CHANNELER_END_ERRORS

#undef CHANNELER_ERROR_FUNCTIONS

#endif // guard
