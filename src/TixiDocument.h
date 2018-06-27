#pragma once

#include "TixiHelper.h"

namespace tigl {
    namespace tixihelper {
        class TixiDocument {
            static const TixiDocumentHandle invalidTixiHandle = -1;
        public:
            TixiDocument() = default;

            explicit TixiDocument(TixiDocumentHandle handle)
                : m_handle(handle) {}
            
            TixiDocument(const TixiDocument&) = delete;
            TixiDocument& operator=(const TixiDocument&) = delete;

            TixiDocument(TixiDocument&& other) noexcept {
                swap(other);
            }

            TixiDocument& operator=(TixiDocument&& other) noexcept {
                swap(other);
                return *this;
            }

            ~TixiDocument() {
                if (m_handle != invalidTixiHandle)
                    tixiCloseDocument(m_handle);
            }

            static TixiDocument createFromFile(const std::string& filename) {
                TixiDocument doc(tixi::TixiOpenDocument(filename));
                return doc;
            }

            static TixiDocument createNew(const std::string& rootElement) {
                return TixiDocument(tixi::TixiCreateDocument(rootElement));
            }

            static TixiDocument createFromString(const std::string& xml) {
                TixiDocument doc(tixi::TixiImportFromString(xml));
                return doc;
            }

            void addCpacsHeader(const std::string& name, const std::string& creator, const std::string& version, const std::string& description, const std::string& cpacsVersion) {
                tixi::TixiAddCpacsHeader(m_handle, name, creator, version, description, cpacsVersion);
            }

            bool checkAttribute(const std::string& xpath, const std::string& attribute) const {
                return tixi::TixiCheckAttribute(m_handle, xpath, attribute);
            }

            bool checkElement(const std::string& xpath) const {
                return tixi::TixiCheckElement(m_handle, xpath);
            }

            int namedChildCount(const std::string& xpath) const {
                return tixi::TixiGetNamedChildrenCount(m_handle, xpath);
            }

            std::vector<std::string> attributeNames(const std::string& xpath) const {
                return tixi::TixiGetAttributeNames(m_handle, xpath);
            }

            template <typename Func>
            void forEachChild(const std::string& xpath, Func func) const {
                const auto count = namedChildCount(xpath);
                for (int i = 1; i <= count; i++)
                    func(xpath + "[" + std::to_string(i) + "]");
            }

            std::string textAttribute(const std::string& xpath, const std::string& attribute) const {
                return tixi::TixiGetTextAttribute(m_handle, xpath, attribute);
            }

            double doubleAttribute(const std::string& xpath, const std::string& attribute) const {
                return tixi::TixiGetDoubleAttribute(m_handle, xpath, attribute);
            }

            bool boolAttribute(const std::string& xpath, const std::string& attribute) const {
                return tixi::TixiGetBoolAttribute(m_handle, xpath, attribute);
            }

            int intAttribute(const std::string& xpath, const std::string& attribute) const {
                return tixi::TixiGetIntegerAttribute(m_handle, xpath, attribute);
            }

            std::string textElement(const std::string& xpath) const {
                return tixi::TixiGetTextElement(m_handle, xpath);
            }

            double doubleElement(const std::string& xpath) const {
                return tixi::TixiGetDoubleElement(m_handle, xpath);
            }

            bool boolElement(const std::string& xpath) const {
                return tixi::TixiGetBoolElement(m_handle, xpath);
            }

            int intElement(const std::string& xpath) const {
                return tixi::TixiGetIntElement(m_handle, xpath);
            }

            std::time_t timeTElement(const std::string& xpath) const {
                return tixi::TixiGetTimeTElement(m_handle, xpath);
            }

            void saveAttribute(const std::string& xpath, const std::string& attribute, const char* value) {
                return tixi::TixiSaveAttribute(m_handle, xpath, attribute, value);
            }

            void saveAttribute(const std::string& xpath, const std::string& attribute, const std::string& value) {
                return tixi::TixiSaveAttribute(m_handle, xpath, attribute, value);
            }

            void saveAttribute(const std::string& xpath, const std::string& attribute, double value) {
                return tixi::TixiSaveAttribute(m_handle, xpath, attribute, value);
            }

            void saveAttribute(const std::string& xpath, const std::string& attribute, bool value) {
                return tixi::TixiSaveAttribute(m_handle, xpath, attribute, value);
            }

            void saveAttribute(const std::string& xpath, const std::string& attribute, int value) {
                return tixi::TixiSaveAttribute(m_handle, xpath, attribute, value);
            }

            void saveElement(const std::string& xpath, const char* value) {
                return tixi::TixiSaveElement(m_handle, xpath, value);
            }

            void saveElement(const std::string& xpath, const std::string& value) {
                return tixi::TixiSaveElement(m_handle, xpath, value);
            }

            void saveElement(const std::string& xpath, double value, const std::string& format = "%g") {
                return tixi::TixiSaveElement(m_handle, xpath, value, format);
            }

            void saveElement(const std::string& xpath, bool value) {
                return tixi::TixiSaveElement(m_handle, xpath, value);
            }

            void saveElement(const std::string& xpath, int value) {
                return tixi::TixiSaveElement(m_handle, xpath, value);
            }

            void saveElement(const std::string& xpath, std::time_t value) {
                return tixi::TixiSaveElement(m_handle, xpath, value);
            }

            void createElement(const std::string& xpath) {
                tixi::TixiCreateElement(m_handle, xpath);
            }

            void createElementIfNotExists(const std::string& xpath) {
                tixi::TixiCreateElementIfNotExists(m_handle, xpath);
            }

            // includes creation of all parent elements
            void createElementsNotExists(const std::string& xpath) {
                tixi::TixiCreateElementsIfNotExists(m_handle, xpath);
            }

            void removeAttribute(const std::string& xpath, const std::string& attribute) {
                tixi::TixiRemoveAttribute(m_handle, xpath, attribute);
            }

            void removeElement(const std::string& xpath) {
                tixi::TixiRemoveElement(m_handle, xpath);
            }

            std::string toString() const {
                return tixi::TixiExportDocumentAsString(m_handle);
            }

            void registerNamespaces() {
                tixi::TixiRegisterNamespacesFromDocument(m_handle);
            }

            void registerNamespace(const std::string& namespaceURI, const std::string& prefix) {
                tixi::TixiRegisterNamespace(m_handle, namespaceURI, prefix);
            }

            const TixiDocumentHandle& handle() const {
                return m_handle;
            }

            TixiDocumentHandle releaseHandle() {
                const auto h =  m_handle;
                m_handle = invalidTixiHandle;
                return h;
            }

        private:
            void swap(TixiDocument& other) noexcept {
                using std::swap;
                swap(m_handle, other.m_handle);
            }

            TixiDocumentHandle m_handle = invalidTixiHandle;
        };
    }
}
