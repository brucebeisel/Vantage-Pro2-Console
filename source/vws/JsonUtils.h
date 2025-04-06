#ifndef JSON_UTILS_H_
#define JSON_UTILS_H_

#include <string>
#include <vector>
#include "json.hpp"

namespace vws {

/**
 * Class that contains static utility methods for JSON processing.
 */
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
     * @param root              The root of the JSON DOM in which to search
     * @param name              The name of the element for which to search
     * @param value             The vector into which the array elements will be added
     * @param requiredArraySize If not zero, the exact size of the expected array
     * @return True of the element is found and the array size matches the required size
     */
    template<typename T> static bool findJsonVector(nlohmann::json root, const std::string & name, std::vector<T> & array, int requiredArraySize = 0) {
        array.clear();
        bool success = false;
        auto jlist = root.find(name);

        if (jlist != root.end()) {
            if (requiredArraySize == 0 || (*jlist).size() == requiredArraySize) {
                std::copy((*jlist).begin(), (*jlist).end(), back_inserter(array));
                success = true;
            }
        }

        return success;
    }

    /**
     * Find a JSON array by name in the tree.
     *
     * @param root              The root of the JSON DOM in which to search
     * @param name              The name of the element for which to search
     * @param value             The vector into which the array elements will be added
     * @param requiredArraySize If not zero, the exact size of the expected array
     * @return True of the element is found and the array size matches the required size
     */
    template<typename T> static bool findJsonArray(nlohmann::json root, const std::string & name, T * array, int requiredArraySize = 0) {
        std::vector<T> tempArray;
        if (findJsonVector(root, name, tempArray, requiredArraySize)) {
            std::copy(tempArray.begin(), tempArray.end(), array);
            return true;
        }
        else
            return false;
    }

    /**
     * Pull out the key and value from the given JSON object.
     *
     * @param [in]  object The object from which to pull the key/value pair
     * @param [out] key    The key of the JSON object
     * @param [out] value  The value of the JSON object
     */
    static void extractJsonKeyValue(const nlohmann::json object, std::string & key, std::string & value) {
        auto iterator = object.begin();
        key = iterator.key();
        value = iterator.value();
    }
};

}

#endif
