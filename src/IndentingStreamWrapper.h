#pragma once

#include <ostream>

namespace tigl {
    struct Scope;

    struct EmptyLineTag {};
    constexpr EmptyLineTag EmptyLine;

    class IndentingStreamWrapper {
    public:
        IndentingStreamWrapper(std::ostream& os)
            : os(os) {}

        // indents on first use
        template<typename T>
        friend auto operator<<(IndentingStreamWrapper& isw, T&& t) -> std::ostream& {
            auto& os = isw.noIndent();

            // indentation
            for (unsigned int i = 0; i < isw.level; i++)
                os << "    ";

            // write
            os << std::forward<T>(t);

            // just return raw stream
            return os;
        }

        friend auto operator<<(IndentingStreamWrapper& isw, EmptyLineTag) -> IndentingStreamWrapper& {
            isw.noIndent();
            return isw;
        }

        auto contLine() -> std::ostream& {
            return os;
        }

        auto noIndent() -> std::ostream& {
            // finish last line if we have written something before
            if (os.tellp() > 0)
                os << '\n';
            return os;
        }

    private:
        friend struct Scope;

        unsigned int level = 0;
        std::ostream& os;
    };

    struct Scope {
        Scope(IndentingStreamWrapper& isw)
            : isw(isw) {
            isw.level++;
        }

        ~Scope() {
            isw.level--;
        }

    private:
        IndentingStreamWrapper& isw;
    };
}
