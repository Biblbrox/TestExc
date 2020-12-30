#ifndef UNTITLED2_UTILS_HPP
#define UNTITLED2_UTILS_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace sig
{
    std::vector<std::string> get_lines(std::ifstream& file, int64_t n);

    std::vector<glm::vec2>
    parse_lines(const std::vector<std::string>& lines);

    glm::vec4 find_min_max(const std::string& file);

    void
    rescale(std::vector<glm::vec2>& signal,
            int old_min,  int old_max,
            int new_min, int new_max);

    std::vector<glm::vec2>
    sort_pairs(const std::vector<glm::vec2>& pairs);

    size_t lines_count(const std::string& file_name);
}

#endif //UNTITLED2_UTILS_HPP
