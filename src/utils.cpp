#include <vector>
#include <fstream>
#include <glm/glm.hpp>
#include <algorithm>
#include <iostream>
#include <numeric>

#include "../include/utils.hpp"

/**
 * Читает n строк
 * @param file
 * @param n
 * @return
 */
std::vector<std::string>
sig::get_lines(std::ifstream& file, int64_t n)
{
    assert(file.is_open() && "Unable to open file");
    std::string str;
    std::vector<std::string> lines;
    lines.reserve(n);

    while (std::getline(file, str) && --n >= 0) {
        if (str.empty())
            break;

        lines.push_back(str);
    }

    lines.shrink_to_fit();
    return lines;
}

/**
 * Возвращает вектор с численными значениями каналов
 * @param lines
 * @return
 */
std::vector<glm::vec2>
sig::parse_lines(const std::vector<std::string>& lines)
{
    std::vector<glm::vec2> res;
    res.reserve(lines.size());

    std::string del = ";";
    std::string par, per;
    for (auto& line: lines) {
        std::string str = line;
        size_t pos_del = str.find(del);
        par = str.substr(0, pos_del);
        str.erase(0, pos_del + del.length());

        pos_del = str.find(del);
        per = str.substr(0, pos_del);

        res.emplace_back(std::stod(par), std::stod(per));
    }

    res.shrink_to_fit();
    return res;
}

/**
 * Возвращает vec4(min_par, max_par, min_per, max_per) значения сигнала
 * @param file_name
 * @return
 */
glm::vec4 sig::find_min_max(const std::string& file_name)
{
    const int buffer_size = 256;
    std::ifstream file(file_name);
    if (!file.good())
        throw std::runtime_error("Unable to open file: " + file_name);

    std::vector<std::string> lines;
    double min_par;
    double min_per;
    double max_par;
    double max_per;
    bool first = true;
    while (!(lines = get_lines(file, buffer_size)).empty()) {
        std::vector<glm::vec2> lines_val = parse_lines(lines);
        if (first) {
            min_par = lines[0][0];
            min_per = lines[0][1];
            max_par = lines[0][0];
            max_per = lines[0][1];
            first = false;
        }
        for (const auto &line: lines_val) {
            double par = line[0];
            double per = line[1];

            if (par < min_par)
                min_par = par;
            if (par > max_par)
                max_par = par;
            if (per < min_per)
                min_per = per;
            if (per > max_per)
                max_per = per;
        }
    }

    file.close();

    return {min_par, max_par, min_per, max_per};
}

/**
 * Нормирование сигнала на новый диапазон
 * @param signal
 * @param old_min
 * @param old_max
 * @param new_min
 * @param new_max
 */
void sig::rescale(std::vector<glm::vec2>& signal,
                  int old_min, int old_max,
                  int new_min, int new_max)
{
    std::vector<glm::vec2> scaled;
    scaled.reserve(signal.size());
    for (auto& line: signal) {
        line = glm::vec2((line[0] - static_cast<double>(old_min)) // Left channel
                         / (old_max - old_min)
                         * (new_max - new_min) + new_min,
                         (line[1] - static_cast<double>(old_min)) // Right channel
                         / (old_max - old_min)
                         * (new_max - new_min) + new_min);
    }
}

/**
 * Независимая сортировка каналов
 * @param pairs
 * @return
 */
std::vector<glm::vec2>
sig::sort_pairs(const std::vector<glm::vec2>& pairs)
{
    std::vector<int> indicies1(pairs.size());
    std::vector<int> indicies2(pairs.size());
    std::iota(indicies1.begin(), indicies1.end(), 0);
    std::iota(indicies2.begin(), indicies2.end(), 0);

    // Сортировка индексов вместо элементов
    std::stable_sort(indicies1.begin(), indicies1.end(),
                     [&pairs](size_t i, size_t j){ return pairs[i][0] < pairs[j][0]; });
    std::stable_sort(indicies2.begin(), indicies2.end(),
                     [&pairs](size_t i, size_t j){ return pairs[i][1] < pairs[j][1]; });

    std::vector<glm::vec2> res;
    res.reserve(pairs.size());
    for (size_t i = 0, j = 0; i < indicies1.size(); ++i, ++j)
        res.emplace_back(pairs[i][0], pairs[j][1]);

    res.shrink_to_fit();
    return res;
}

size_t sig::lines_count(const std::string& file_name)
{
    std::ifstream file(file_name);
    if (!file.good())
        throw std::runtime_error("Unable to open file: " + file_name);
    std::string str;
    size_t count = 0;
    while (std::getline(file, str)) ++count;
    file.close();

    return count;
}