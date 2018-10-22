// Copyright (c) 2018 RISC Software GmbH
//
// This file was generated by CPACSGen from CPACS XML Schema (c) German Aerospace Center (DLR/SC).
// Do not edit, all changes are lost when files are re-generated.
//
// Licensed under the Apache License, Version 2.0 (the "License")
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <string>
#include <tixi.h>
#include <vector>
#include "tigl_internal.h"

namespace tigl
{
namespace generated
{
    // This class is used in:
    // generated from /xsd:schema/xsd:complexType[1]
    class CPACSRoot
    {
    public:
        TIGL_EXPORT CPACSRoot();
        TIGL_EXPORT virtual ~CPACSRoot();

        TIGL_EXPORT virtual void ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath);
        TIGL_EXPORT virtual void WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const;

        TIGL_EXPORT virtual const int& GetA() const;
        TIGL_EXPORT virtual void SetA(const int& value);

        TIGL_EXPORT virtual const boost::optional<int>& GetB() const;
        TIGL_EXPORT virtual void SetB(const boost::optional<int>& value);

        TIGL_EXPORT virtual const int& GetC() const;
        TIGL_EXPORT virtual void SetC(const int& value);

        TIGL_EXPORT virtual const int& GetD() const;
        TIGL_EXPORT virtual void SetD(const int& value);

        TIGL_EXPORT virtual const std::vector<int>& GetEs() const;
        TIGL_EXPORT virtual std::vector<int>& GetEs();

        TIGL_EXPORT virtual const boost::optional<int>& GetF() const;
        TIGL_EXPORT virtual void SetF(const boost::optional<int>& value);

        TIGL_EXPORT virtual const std::vector<int>& GetGs() const;
        TIGL_EXPORT virtual std::vector<int>& GetGs();

        TIGL_EXPORT virtual const int& GetH() const;
        TIGL_EXPORT virtual void SetH(const int& value);

        TIGL_EXPORT virtual const std::vector<int>& GetIs() const;
        TIGL_EXPORT virtual std::vector<int>& GetIs();

    protected:
        int                  m_a;
        boost::optional<int> m_b;
        int                  m_c;
        int                  m_d;
        std::vector<int>     m_es;
        boost::optional<int> m_f;
        std::vector<int>     m_gs;
        int                  m_h;
        std::vector<int>     m_is;

    private:
#ifdef HAVE_CPP11
        CPACSRoot(const CPACSRoot&) = delete;
        CPACSRoot& operator=(const CPACSRoot&) = delete;

        CPACSRoot(CPACSRoot&&) = delete;
        CPACSRoot& operator=(CPACSRoot&&) = delete;
#else
        CPACSRoot(const CPACSRoot&);
        CPACSRoot& operator=(const CPACSRoot&);
#endif
    };
} // namespace generated

// Aliases in tigl namespace
#ifdef HAVE_CPP11
using CCPACSRoot = generated::CPACSRoot;
#else
typedef generated::CPACSRoot CCPACSRoot;
#endif
} // namespace tigl
// Copyright (c) 2018 RISC Software GmbH
//
// This file was generated by CPACSGen from CPACS XML Schema (c) German Aerospace Center (DLR/SC).
// Do not edit, all changes are lost when files are re-generated.
//
// Licensed under the Apache License, Version 2.0 (the "License")
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "CPACSRoot.h"
#include "CTiglError.h"
#include "CTiglLogging.h"
#include "TixiHelper.h"

namespace tigl
{
namespace generated
{
    CPACSRoot::CPACSRoot()
        : m_a(0)
        , m_c(0)
        , m_d(0)
        , m_h(0)
    {
    }

    CPACSRoot::~CPACSRoot()
    {
    }

    void CPACSRoot::ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath)
    {
        // read element a
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/a")) {
            m_a = tixi::TixiGetElement<int>(tixiHandle, xpath + "/a");
        }
        else {
            LOG(ERROR) << "Required element a is missing at xpath " << xpath;
        }

        // read element b
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/b")) {
            m_b = tixi::TixiGetElement<int>(tixiHandle, xpath + "/b");
        }

        // read element c
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/c")) {
            m_c = tixi::TixiGetElement<int>(tixiHandle, xpath + "/c");
        }
        else {
            LOG(ERROR) << "Required element c is missing at xpath " << xpath;
        }

        // read element d
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/d")) {
            m_d = tixi::TixiGetElement<int>(tixiHandle, xpath + "/d");
        }
        else {
            LOG(ERROR) << "Required element d is missing at xpath " << xpath;
        }

        // read element e
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/e")) {
            tixi::TixiReadElements(tixiHandle, xpath + "/e", m_es);
        }

        // read element f
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/f")) {
            m_f = tixi::TixiGetElement<int>(tixiHandle, xpath + "/f");
        }

        // read element g
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/g")) {
            tixi::TixiReadElements(tixiHandle, xpath + "/g", m_gs);
        }

        // read element h
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/h")) {
            m_h = tixi::TixiGetElement<int>(tixiHandle, xpath + "/h");
        }
        else {
            LOG(ERROR) << "Required element h is missing at xpath " << xpath;
        }

        // read element i
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/i")) {
            tixi::TixiReadElements(tixiHandle, xpath + "/i", m_is);
        }

    }

    void CPACSRoot::WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const
    {
        // write element a
        tixi::TixiCreateElementIfNotExists(tixiHandle, xpath + "/a");
        tixi::TixiSaveElement(tixiHandle, xpath + "/a", m_a);

        // write element b
        if (m_b) {
            tixi::TixiCreateElementIfNotExists(tixiHandle, xpath + "/b");
            tixi::TixiSaveElement(tixiHandle, xpath + "/b", *m_b);
        }
        else {
            if (tixi::TixiCheckElement(tixiHandle, xpath + "/b")) {
                tixi::TixiRemoveElement(tixiHandle, xpath + "/b");
            }
        }

        // write element c
        tixi::TixiCreateElementIfNotExists(tixiHandle, xpath + "/c");
        tixi::TixiSaveElement(tixiHandle, xpath + "/c", m_c);

        // write element d
        tixi::TixiCreateElementIfNotExists(tixiHandle, xpath + "/d");
        tixi::TixiSaveElement(tixiHandle, xpath + "/d", m_d);

        // write element e
        tixi::TixiSaveElements(tixiHandle, xpath + "/e", m_es);

        // write element f
        if (m_f) {
            tixi::TixiCreateElementIfNotExists(tixiHandle, xpath + "/f");
            tixi::TixiSaveElement(tixiHandle, xpath + "/f", *m_f);
        }
        else {
            if (tixi::TixiCheckElement(tixiHandle, xpath + "/f")) {
                tixi::TixiRemoveElement(tixiHandle, xpath + "/f");
            }
        }

        // write element g
        tixi::TixiSaveElements(tixiHandle, xpath + "/g", m_gs);

        // write element h
        tixi::TixiCreateElementIfNotExists(tixiHandle, xpath + "/h");
        tixi::TixiSaveElement(tixiHandle, xpath + "/h", m_h);

        // write element i
        tixi::TixiSaveElements(tixiHandle, xpath + "/i", m_is);

    }

    const int& CPACSRoot::GetA() const
    {
        return m_a;
    }

    void CPACSRoot::SetA(const int& value)
    {
        m_a = value;
    }

    const boost::optional<int>& CPACSRoot::GetB() const
    {
        return m_b;
    }

    void CPACSRoot::SetB(const boost::optional<int>& value)
    {
        m_b = value;
    }

    const int& CPACSRoot::GetC() const
    {
        return m_c;
    }

    void CPACSRoot::SetC(const int& value)
    {
        m_c = value;
    }

    const int& CPACSRoot::GetD() const
    {
        return m_d;
    }

    void CPACSRoot::SetD(const int& value)
    {
        m_d = value;
    }

    const std::vector<int>& CPACSRoot::GetEs() const
    {
        return m_es;
    }

    std::vector<int>& CPACSRoot::GetEs()
    {
        return m_es;
    }

    const boost::optional<int>& CPACSRoot::GetF() const
    {
        return m_f;
    }

    void CPACSRoot::SetF(const boost::optional<int>& value)
    {
        m_f = value;
    }

    const std::vector<int>& CPACSRoot::GetGs() const
    {
        return m_gs;
    }

    std::vector<int>& CPACSRoot::GetGs()
    {
        return m_gs;
    }

    const int& CPACSRoot::GetH() const
    {
        return m_h;
    }

    void CPACSRoot::SetH(const int& value)
    {
        m_h = value;
    }

    const std::vector<int>& CPACSRoot::GetIs() const
    {
        return m_is;
    }

    std::vector<int>& CPACSRoot::GetIs()
    {
        return m_is;
    }

} // namespace generated
} // namespace tigl
