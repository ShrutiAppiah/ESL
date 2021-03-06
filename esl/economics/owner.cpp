/// \file   owner.cpp
///
/// \brief  Allows an agent to hold property of a specific type, and allows one
///         to write specialised processing of property of this type
///
/// \authors    Maarten P. Scholl
/// \date       2019-01-21
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
#include <boost/serialization/export.hpp>

#include <esl/economics/owner.hpp>
#include <esl/data/log.hpp>


namespace esl::law {

}


//BOOST_CLASS_TRACKING(esl::law::owner<esl::law::property>, boost::serialization::track_always)
//;
//BOOST_CLASS_EXPORT(esl::law::owner<esl::law::property>);