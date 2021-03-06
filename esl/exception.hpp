/// \file   exception.hpp
///
/// \brief
///
/// \authors    Maarten P. Scholl
/// \date       2019-09-24
/// \copyright  Copyright 2017-2019 The Institute for New Economic Thinking,
///             Oxford Martin School, University of Oxford
///
///             Licensed under the Apache License, Version 2.0 (the "License");
///             you may not use this file except in compliance with the License.
///             You may obtain a copy of the License at
///
///                 http://www.apache.org/licenses/LICENSE-2.0
///
///             Unless required by applicable law or agreed to in writing,
///             software distributed under the License is distributed on an "AS
///             IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
///             express or implied. See the License for the specific language
///             governing permissions and limitations under the License.
///
///             You may obtain instructions to fulfill the attribution
///             requirements in CITATION.cff
///
#ifndef ESL_EXCEPTION_HPP
#define ESL_EXCEPTION_HPP

#include <exception>
#include <string>


namespace esl {
    ///
    /// \brief  The base class for exception raised from the library.
    ///
    /// \details    This class is implemented to allow filtering out library
    ///             exceptions from other exceptions. Additionally, this allows
    ///             us to translate library exceptions to exceptions or error
    ///             codes in language bindings, such as Python exceptions.
    ///
    class exception
    : public std::exception
    {
    protected:
        std::string message_;

    public:
        explicit exception(const std::string &message = std::string())
        : message_(message)
        {

        }

        ~exception() override = default;

        [[nodiscard]] const char *what() const noexcept override
        {
            return message_.c_str();
        }
    };
}


#endif  // ESL_EXCEPTION_HPP
