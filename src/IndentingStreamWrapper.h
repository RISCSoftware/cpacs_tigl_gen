#pragma once

#include <ostream>

namespace tigl {
    struct Scope;

    class IndentingStreamWrapper {
    public:
        IndentingStreamWrapper(std::ostream& os)
            : os(os) {}

        // indents on first use
        template<typename T>
        friend auto operator<<(IndentingStreamWrapper& isw, T&& t) -> std::ostream& {
            // finish last line if we have written something before
            if (isw.os.tellp() > 0)
                isw.os << '\n';

            // indentation
            for (unsigned int i = 0; i < isw.level; i++)
                isw.os << "    ";

            // write
            isw.os << std::forward<T>(t);

            // just return the unterlying stream
            return isw.os;
        }

        auto contLine() -> std::ostream& {
            return os;
        }

        auto noIndent() -> std::ostream& {
            // finish last line
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
