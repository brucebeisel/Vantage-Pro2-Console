#ifndef JSON_UTILS_H_
#define JSON_UTILS_H_

#include <string>
#include "json.hpp"

//using json = nlohmann::json;

namespace vws {

class JsonUtils {
public:
    /**
     * Find a JSON element by name in the tree.
     *
     * @param root  The root of the JSON DOM in which to search
     * @param name  The name of the element for which to search
     * @param value The value of the element if found
     * @return True of the element is found
     */
    template<typename T> static bool findJsonValue(nlohmann::json root, const std::string & name, T & value) {
        bool success = false;
        auto valuePtr = root.find(name);
        if (valuePtr != root.end()) {
            value = *valuePtr;
            success = true;
        }

        return success;
    }

    /**
     * Find a JSON array by name in the tree.
     *
     * @param root  The root of the JSON DOM in which to search
     * @param name  The name of the element for which to search
     * @param value The value of the array if found
     * @return True of the element is found
     */
    template<typename T> static bool findJsonArray(nlohmann::json root, const std::string & name, T & array, int size = 0) {
        bool success = false;
        auto jlist = root.find(name);

        if (jlist != root.end()) {
            if (size == 0 || (*jlist).size() == size) {
                std::copy((*jlist).begin(), (*jlist).end(), array);
                success = true;
            }
        }

        return success;
    }
};

}

#endif
