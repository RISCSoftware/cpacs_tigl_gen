// Copyright (c) 2020 RISC Software GmbH
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
#include "tigl_internal.h"

namespace tigl
{
class CTiglUIDObject;

namespace generated
{
    class CPACSRoot;

    // This class is used in:
    // CPACSRoot

    class CPACSDoubleVectorBase
    {
    public:
        TIGL_EXPORT CPACSDoubleVectorBase(CPACSRoot* parent);

        TIGL_EXPORT virtual ~CPACSDoubleVectorBase();

        TIGL_EXPORT CPACSRoot* GetParent();

        TIGL_EXPORT const CPACSRoot* GetParent() const;

        TIGL_EXPORT virtual CTiglUIDObject* GetNextUIDParent();
        TIGL_EXPORT virtual const CTiglUIDObject* GetNextUIDParent() const;

        TIGL_EXPORT virtual void ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath);
        TIGL_EXPORT virtual void WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const;

        TIGL_EXPORT virtual const boost::optional<std::string>& GetMapType() const;
        TIGL_EXPORT virtual void SetMapType(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetMu() const;
        TIGL_EXPORT virtual void SetMu(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetDelta() const;
        TIGL_EXPORT virtual void SetDelta(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetA() const;
        TIGL_EXPORT virtual void SetA(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetB() const;
        TIGL_EXPORT virtual void SetB(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetC() const;
        TIGL_EXPORT virtual void SetC(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetV() const;
        TIGL_EXPORT virtual void SetV(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetW() const;
        TIGL_EXPORT virtual void SetW(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const std::string& GetValue() const;
        TIGL_EXPORT virtual void SetValue(const std::string& value);

    protected:
        CPACSRoot* m_parent;

        boost::optional<std::string> m_mapType;
        boost::optional<std::string> m_mu;
        boost::optional<std::string> m_delta;
        boost::optional<std::string> m_a;
        boost::optional<std::string> m_b;
        boost::optional<std::string> m_c;
        boost::optional<std::string> m_v;
        boost::optional<std::string> m_w;
        std::string                  m_value;

    private:
        CPACSDoubleVectorBase(const CPACSDoubleVectorBase&) = delete;
        CPACSDoubleVectorBase& operator=(const CPACSDoubleVectorBase&) = delete;

        CPACSDoubleVectorBase(CPACSDoubleVectorBase&&) = delete;
        CPACSDoubleVectorBase& operator=(CPACSDoubleVectorBase&&) = delete;
    };
} // namespace generated

// Aliases in tigl namespace
using CCPACSDoubleVectorBase = generated::CPACSDoubleVectorBase;
using CCPACSRoot = generated::CPACSRoot;
} // namespace tigl
// Copyright (c) 2020 RISC Software GmbH
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
#include "CPACSDoubleVectorBase.h"
#include "CPACSStringVectorBase.h"
#include "CreateIfNotExists.h"
#include "tigl_internal.h"

namespace tigl
{
class CTiglUIDObject;

namespace generated
{
    // This class is used in:
    class CPACSRoot
    {
    public:
        TIGL_EXPORT CPACSRoot();
        TIGL_EXPORT virtual ~CPACSRoot();

        TIGL_EXPORT virtual CTiglUIDObject* GetNextUIDParent();
        TIGL_EXPORT virtual const CTiglUIDObject* GetNextUIDParent() const;

        TIGL_EXPORT virtual void ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath);
        TIGL_EXPORT virtual void WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const;

        TIGL_EXPORT virtual const boost::optional<CPACSStringVectorBase>& GetStringVector() const;
        TIGL_EXPORT virtual boost::optional<CPACSStringVectorBase>& GetStringVector();

        TIGL_EXPORT virtual const boost::optional<CPACSDoubleVectorBase>& GetDoubleVector() const;
        TIGL_EXPORT virtual boost::optional<CPACSDoubleVectorBase>& GetDoubleVector();

        TIGL_EXPORT virtual CPACSStringVectorBase& GetStringVector(CreateIfNotExistsTag);
        TIGL_EXPORT virtual void RemoveStringVector();

        TIGL_EXPORT virtual CPACSDoubleVectorBase& GetDoubleVector(CreateIfNotExistsTag);
        TIGL_EXPORT virtual void RemoveDoubleVector();

    protected:
        boost::optional<CPACSStringVectorBase> m_stringVector;
        boost::optional<CPACSDoubleVectorBase> m_doubleVector;

    private:
        CPACSRoot(const CPACSRoot&) = delete;
        CPACSRoot& operator=(const CPACSRoot&) = delete;

        CPACSRoot(CPACSRoot&&) = delete;
        CPACSRoot& operator=(CPACSRoot&&) = delete;
    };
} // namespace generated

// Aliases in tigl namespace
using CCPACSRoot = generated::CPACSRoot;
} // namespace tigl
// Copyright (c) 2020 RISC Software GmbH
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
#include "tigl_internal.h"

namespace tigl
{
class CTiglUIDObject;

namespace generated
{
    class CPACSRoot;

    // This class is used in:
    // CPACSRoot

    class CPACSStringVectorBase
    {
    public:
        TIGL_EXPORT CPACSStringVectorBase(CPACSRoot* parent);

        TIGL_EXPORT virtual ~CPACSStringVectorBase();

        TIGL_EXPORT CPACSRoot* GetParent();

        TIGL_EXPORT const CPACSRoot* GetParent() const;

        TIGL_EXPORT virtual CTiglUIDObject* GetNextUIDParent();
        TIGL_EXPORT virtual const CTiglUIDObject* GetNextUIDParent() const;

        TIGL_EXPORT virtual void ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath);
        TIGL_EXPORT virtual void WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const;

        TIGL_EXPORT virtual const boost::optional<std::string>& GetMapType() const;
        TIGL_EXPORT virtual void SetMapType(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetMu() const;
        TIGL_EXPORT virtual void SetMu(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetDelta() const;
        TIGL_EXPORT virtual void SetDelta(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetA() const;
        TIGL_EXPORT virtual void SetA(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetB() const;
        TIGL_EXPORT virtual void SetB(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetC() const;
        TIGL_EXPORT virtual void SetC(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetV() const;
        TIGL_EXPORT virtual void SetV(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const boost::optional<std::string>& GetW() const;
        TIGL_EXPORT virtual void SetW(const boost::optional<std::string>& value);

        TIGL_EXPORT virtual const std::string& GetValue() const;
        TIGL_EXPORT virtual void SetValue(const std::string& value);

    protected:
        CPACSRoot* m_parent;

        boost::optional<std::string> m_mapType;
        boost::optional<std::string> m_mu;
        boost::optional<std::string> m_delta;
        boost::optional<std::string> m_a;
        boost::optional<std::string> m_b;
        boost::optional<std::string> m_c;
        boost::optional<std::string> m_v;
        boost::optional<std::string> m_w;
        std::string                  m_value;

    private:
        CPACSStringVectorBase(const CPACSStringVectorBase&) = delete;
        CPACSStringVectorBase& operator=(const CPACSStringVectorBase&) = delete;

        CPACSStringVectorBase(CPACSStringVectorBase&&) = delete;
        CPACSStringVectorBase& operator=(CPACSStringVectorBase&&) = delete;
    };
} // namespace generated

// Aliases in tigl namespace
using CCPACSStringVectorBase = generated::CPACSStringVectorBase;
using CCPACSRoot = generated::CPACSRoot;
} // namespace tigl
// Copyright (c) 2020 RISC Software GmbH
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

#include <cassert>
#include "CPACSDoubleVectorBase.h"
#include "CPACSRoot.h"
#include "CTiglError.h"
#include "CTiglLogging.h"
#include "CTiglUIDObject.h"
#include "TixiHelper.h"

namespace tigl
{
namespace generated
{
    CPACSDoubleVectorBase::CPACSDoubleVectorBase(CPACSRoot* parent)
    {
        //assert(parent != NULL);
        m_parent = parent;
    }

    CPACSDoubleVectorBase::~CPACSDoubleVectorBase()
    {
    }

    const CPACSRoot* CPACSDoubleVectorBase::GetParent() const
    {
        return m_parent;
    }

    CPACSRoot* CPACSDoubleVectorBase::GetParent()
    {
        return m_parent;
    }

    const CTiglUIDObject* CPACSDoubleVectorBase::GetNextUIDParent() const
    {
        if (m_parent) {
            return m_parent->GetNextUIDParent();
        }
        return nullptr;
    }

    CTiglUIDObject* CPACSDoubleVectorBase::GetNextUIDParent()
    {
        if (m_parent) {
            return m_parent->GetNextUIDParent();
        }
        return nullptr;
    }

    void CPACSDoubleVectorBase::ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath)
    {
        // read attribute mapType
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "mapType")) {
            m_mapType = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "mapType");
            if (m_mapType->empty()) {
                LOG(WARNING) << "Optional attribute mapType is present but empty at xpath " << xpath;
            }
        }

        // read attribute mu
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "mu")) {
            m_mu = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "mu");
            if (m_mu->empty()) {
                LOG(WARNING) << "Optional attribute mu is present but empty at xpath " << xpath;
            }
        }

        // read attribute delta
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "delta")) {
            m_delta = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "delta");
            if (m_delta->empty()) {
                LOG(WARNING) << "Optional attribute delta is present but empty at xpath " << xpath;
            }
        }

        // read attribute a
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "a")) {
            m_a = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "a");
            if (m_a->empty()) {
                LOG(WARNING) << "Optional attribute a is present but empty at xpath " << xpath;
            }
        }

        // read attribute b
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "b")) {
            m_b = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "b");
            if (m_b->empty()) {
                LOG(WARNING) << "Optional attribute b is present but empty at xpath " << xpath;
            }
        }

        // read attribute c
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "c")) {
            m_c = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "c");
            if (m_c->empty()) {
                LOG(WARNING) << "Optional attribute c is present but empty at xpath " << xpath;
            }
        }

        // read attribute v
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "v")) {
            m_v = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "v");
            if (m_v->empty()) {
                LOG(WARNING) << "Optional attribute v is present but empty at xpath " << xpath;
            }
        }

        // read attribute w
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "w")) {
            m_w = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "w");
            if (m_w->empty()) {
                LOG(WARNING) << "Optional attribute w is present but empty at xpath " << xpath;
            }
        }

        // read simpleContent 
        if (tixi::TixiCheckElement(tixiHandle, xpath)) {
            m_value = tixi::TixiGetElement<std::string>(tixiHandle, xpath);
            if (m_value.empty()) {
                LOG(WARNING) << "Required element  is empty at xpath " << xpath;
            }
        }
        else {
            LOG(ERROR) << "Required simpleContent  is missing at xpath " << xpath;
        }

    }

    void CPACSDoubleVectorBase::WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const
    {
        // write attribute mapType
        if (m_mapType) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "mapType", *m_mapType);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "mapType")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "mapType");
            }
        }

        // write attribute mu
        if (m_mu) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "mu", *m_mu);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "mu")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "mu");
            }
        }

        // write attribute delta
        if (m_delta) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "delta", *m_delta);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "delta")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "delta");
            }
        }

        // write attribute a
        if (m_a) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "a", *m_a);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "a")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "a");
            }
        }

        // write attribute b
        if (m_b) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "b", *m_b);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "b")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "b");
            }
        }

        // write attribute c
        if (m_c) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "c", *m_c);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "c")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "c");
            }
        }

        // write attribute v
        if (m_v) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "v", *m_v);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "v")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "v");
            }
        }

        // write attribute w
        if (m_w) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "w", *m_w);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "w")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "w");
            }
        }

        // write simpleContent 
        tixi::TixiSaveElement(tixiHandle, xpath, m_value);

    }

    const boost::optional<std::string>& CPACSDoubleVectorBase::GetMapType() const
    {
        return m_mapType;
    }

    void CPACSDoubleVectorBase::SetMapType(const boost::optional<std::string>& value)
    {
        m_mapType = value;
    }

    const boost::optional<std::string>& CPACSDoubleVectorBase::GetMu() const
    {
        return m_mu;
    }

    void CPACSDoubleVectorBase::SetMu(const boost::optional<std::string>& value)
    {
        m_mu = value;
    }

    const boost::optional<std::string>& CPACSDoubleVectorBase::GetDelta() const
    {
        return m_delta;
    }

    void CPACSDoubleVectorBase::SetDelta(const boost::optional<std::string>& value)
    {
        m_delta = value;
    }

    const boost::optional<std::string>& CPACSDoubleVectorBase::GetA() const
    {
        return m_a;
    }

    void CPACSDoubleVectorBase::SetA(const boost::optional<std::string>& value)
    {
        m_a = value;
    }

    const boost::optional<std::string>& CPACSDoubleVectorBase::GetB() const
    {
        return m_b;
    }

    void CPACSDoubleVectorBase::SetB(const boost::optional<std::string>& value)
    {
        m_b = value;
    }

    const boost::optional<std::string>& CPACSDoubleVectorBase::GetC() const
    {
        return m_c;
    }

    void CPACSDoubleVectorBase::SetC(const boost::optional<std::string>& value)
    {
        m_c = value;
    }

    const boost::optional<std::string>& CPACSDoubleVectorBase::GetV() const
    {
        return m_v;
    }

    void CPACSDoubleVectorBase::SetV(const boost::optional<std::string>& value)
    {
        m_v = value;
    }

    const boost::optional<std::string>& CPACSDoubleVectorBase::GetW() const
    {
        return m_w;
    }

    void CPACSDoubleVectorBase::SetW(const boost::optional<std::string>& value)
    {
        m_w = value;
    }

    const std::string& CPACSDoubleVectorBase::GetValue() const
    {
        return m_value;
    }

    void CPACSDoubleVectorBase::SetValue(const std::string& value)
    {
        m_value = value;
    }

} // namespace generated
} // namespace tigl
// Copyright (c) 2020 RISC Software GmbH
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
#include "CTiglUIDObject.h"
#include "TixiHelper.h"

namespace tigl
{
namespace generated
{
    CPACSRoot::CPACSRoot()
    {
    }

    CPACSRoot::~CPACSRoot()
    {
    }

    const CTiglUIDObject* CPACSRoot::GetNextUIDParent() const
    {
        return nullptr;
    }

    CTiglUIDObject* CPACSRoot::GetNextUIDParent()
    {
        return nullptr;
    }

    void CPACSRoot::ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath)
    {
        // read element stringVector
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/stringVector")) {
            m_stringVector = boost::in_place(this);
            try {
                m_stringVector->ReadCPACS(tixiHandle, xpath + "/stringVector");
            } catch(const std::exception& e) {
                LOG(ERROR) << "Failed to read stringVector at xpath " << xpath << ": " << e.what();
                m_stringVector = boost::none;
            }
        }

        // read element doubleVector
        if (tixi::TixiCheckElement(tixiHandle, xpath + "/doubleVector")) {
            m_doubleVector = boost::in_place(this);
            try {
                m_doubleVector->ReadCPACS(tixiHandle, xpath + "/doubleVector");
            } catch(const std::exception& e) {
                LOG(ERROR) << "Failed to read doubleVector at xpath " << xpath << ": " << e.what();
                m_doubleVector = boost::none;
            }
        }

    }

    void CPACSRoot::WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const
    {
        const std::vector<std::string> childElemOrder = { "stringVector", "doubleVector" };

        // write element stringVector
        if (m_stringVector) {
            tixi::TixiCreateSequenceElementIfNotExists(tixiHandle, xpath + "/stringVector", childElemOrder);
            m_stringVector->WriteCPACS(tixiHandle, xpath + "/stringVector");
        }
        else {
            if (tixi::TixiCheckElement(tixiHandle, xpath + "/stringVector")) {
                tixi::TixiRemoveElement(tixiHandle, xpath + "/stringVector");
            }
        }

        // write element doubleVector
        if (m_doubleVector) {
            tixi::TixiCreateSequenceElementIfNotExists(tixiHandle, xpath + "/doubleVector", childElemOrder);
            m_doubleVector->WriteCPACS(tixiHandle, xpath + "/doubleVector");
        }
        else {
            if (tixi::TixiCheckElement(tixiHandle, xpath + "/doubleVector")) {
                tixi::TixiRemoveElement(tixiHandle, xpath + "/doubleVector");
            }
        }

    }

    const boost::optional<CPACSStringVectorBase>& CPACSRoot::GetStringVector() const
    {
        return m_stringVector;
    }

    boost::optional<CPACSStringVectorBase>& CPACSRoot::GetStringVector()
    {
        return m_stringVector;
    }

    const boost::optional<CPACSDoubleVectorBase>& CPACSRoot::GetDoubleVector() const
    {
        return m_doubleVector;
    }

    boost::optional<CPACSDoubleVectorBase>& CPACSRoot::GetDoubleVector()
    {
        return m_doubleVector;
    }

    CPACSStringVectorBase& CPACSRoot::GetStringVector(CreateIfNotExistsTag)
    {
        if (!m_stringVector)
            m_stringVector = boost::in_place(this);
        return *m_stringVector;
    }

    void CPACSRoot::RemoveStringVector()
    {
        m_stringVector = boost::none;
    }

    CPACSDoubleVectorBase& CPACSRoot::GetDoubleVector(CreateIfNotExistsTag)
    {
        if (!m_doubleVector)
            m_doubleVector = boost::in_place(this);
        return *m_doubleVector;
    }

    void CPACSRoot::RemoveDoubleVector()
    {
        m_doubleVector = boost::none;
    }

} // namespace generated
} // namespace tigl
// Copyright (c) 2020 RISC Software GmbH
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

#include <cassert>
#include "CPACSRoot.h"
#include "CPACSStringVectorBase.h"
#include "CTiglError.h"
#include "CTiglLogging.h"
#include "CTiglUIDObject.h"
#include "TixiHelper.h"

namespace tigl
{
namespace generated
{
    CPACSStringVectorBase::CPACSStringVectorBase(CPACSRoot* parent)
    {
        //assert(parent != NULL);
        m_parent = parent;
    }

    CPACSStringVectorBase::~CPACSStringVectorBase()
    {
    }

    const CPACSRoot* CPACSStringVectorBase::GetParent() const
    {
        return m_parent;
    }

    CPACSRoot* CPACSStringVectorBase::GetParent()
    {
        return m_parent;
    }

    const CTiglUIDObject* CPACSStringVectorBase::GetNextUIDParent() const
    {
        if (m_parent) {
            return m_parent->GetNextUIDParent();
        }
        return nullptr;
    }

    CTiglUIDObject* CPACSStringVectorBase::GetNextUIDParent()
    {
        if (m_parent) {
            return m_parent->GetNextUIDParent();
        }
        return nullptr;
    }

    void CPACSStringVectorBase::ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath)
    {
        // read attribute mapType
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "mapType")) {
            m_mapType = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "mapType");
            if (m_mapType->empty()) {
                LOG(WARNING) << "Optional attribute mapType is present but empty at xpath " << xpath;
            }
        }

        // read attribute mu
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "mu")) {
            m_mu = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "mu");
            if (m_mu->empty()) {
                LOG(WARNING) << "Optional attribute mu is present but empty at xpath " << xpath;
            }
        }

        // read attribute delta
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "delta")) {
            m_delta = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "delta");
            if (m_delta->empty()) {
                LOG(WARNING) << "Optional attribute delta is present but empty at xpath " << xpath;
            }
        }

        // read attribute a
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "a")) {
            m_a = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "a");
            if (m_a->empty()) {
                LOG(WARNING) << "Optional attribute a is present but empty at xpath " << xpath;
            }
        }

        // read attribute b
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "b")) {
            m_b = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "b");
            if (m_b->empty()) {
                LOG(WARNING) << "Optional attribute b is present but empty at xpath " << xpath;
            }
        }

        // read attribute c
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "c")) {
            m_c = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "c");
            if (m_c->empty()) {
                LOG(WARNING) << "Optional attribute c is present but empty at xpath " << xpath;
            }
        }

        // read attribute v
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "v")) {
            m_v = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "v");
            if (m_v->empty()) {
                LOG(WARNING) << "Optional attribute v is present but empty at xpath " << xpath;
            }
        }

        // read attribute w
        if (tixi::TixiCheckAttribute(tixiHandle, xpath, "w")) {
            m_w = tixi::TixiGetAttribute<std::string>(tixiHandle, xpath, "w");
            if (m_w->empty()) {
                LOG(WARNING) << "Optional attribute w is present but empty at xpath " << xpath;
            }
        }

        // read simpleContent 
        if (tixi::TixiCheckElement(tixiHandle, xpath)) {
            m_value = tixi::TixiGetElement<std::string>(tixiHandle, xpath);
            if (m_value.empty()) {
                LOG(WARNING) << "Required element  is empty at xpath " << xpath;
            }
        }
        else {
            LOG(ERROR) << "Required simpleContent  is missing at xpath " << xpath;
        }

    }

    void CPACSStringVectorBase::WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const
    {
        // write attribute mapType
        if (m_mapType) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "mapType", *m_mapType);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "mapType")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "mapType");
            }
        }

        // write attribute mu
        if (m_mu) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "mu", *m_mu);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "mu")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "mu");
            }
        }

        // write attribute delta
        if (m_delta) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "delta", *m_delta);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "delta")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "delta");
            }
        }

        // write attribute a
        if (m_a) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "a", *m_a);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "a")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "a");
            }
        }

        // write attribute b
        if (m_b) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "b", *m_b);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "b")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "b");
            }
        }

        // write attribute c
        if (m_c) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "c", *m_c);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "c")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "c");
            }
        }

        // write attribute v
        if (m_v) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "v", *m_v);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "v")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "v");
            }
        }

        // write attribute w
        if (m_w) {
            tixi::TixiSaveAttribute(tixiHandle, xpath, "w", *m_w);
        }
        else {
            if (tixi::TixiCheckAttribute(tixiHandle, xpath, "w")) {
                tixi::TixiRemoveAttribute(tixiHandle, xpath, "w");
            }
        }

        // write simpleContent 
        tixi::TixiSaveElement(tixiHandle, xpath, m_value);

    }

    const boost::optional<std::string>& CPACSStringVectorBase::GetMapType() const
    {
        return m_mapType;
    }

    void CPACSStringVectorBase::SetMapType(const boost::optional<std::string>& value)
    {
        m_mapType = value;
    }

    const boost::optional<std::string>& CPACSStringVectorBase::GetMu() const
    {
        return m_mu;
    }

    void CPACSStringVectorBase::SetMu(const boost::optional<std::string>& value)
    {
        m_mu = value;
    }

    const boost::optional<std::string>& CPACSStringVectorBase::GetDelta() const
    {
        return m_delta;
    }

    void CPACSStringVectorBase::SetDelta(const boost::optional<std::string>& value)
    {
        m_delta = value;
    }

    const boost::optional<std::string>& CPACSStringVectorBase::GetA() const
    {
        return m_a;
    }

    void CPACSStringVectorBase::SetA(const boost::optional<std::string>& value)
    {
        m_a = value;
    }

    const boost::optional<std::string>& CPACSStringVectorBase::GetB() const
    {
        return m_b;
    }

    void CPACSStringVectorBase::SetB(const boost::optional<std::string>& value)
    {
        m_b = value;
    }

    const boost::optional<std::string>& CPACSStringVectorBase::GetC() const
    {
        return m_c;
    }

    void CPACSStringVectorBase::SetC(const boost::optional<std::string>& value)
    {
        m_c = value;
    }

    const boost::optional<std::string>& CPACSStringVectorBase::GetV() const
    {
        return m_v;
    }

    void CPACSStringVectorBase::SetV(const boost::optional<std::string>& value)
    {
        m_v = value;
    }

    const boost::optional<std::string>& CPACSStringVectorBase::GetW() const
    {
        return m_w;
    }

    void CPACSStringVectorBase::SetW(const boost::optional<std::string>& value)
    {
        m_w = value;
    }

    const std::string& CPACSStringVectorBase::GetValue() const
    {
        return m_value;
    }

    void CPACSStringVectorBase::SetValue(const std::string& value)
    {
        m_value = value;
    }

} // namespace generated
} // namespace tigl
