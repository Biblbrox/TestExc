#include <iostream>
#include <fstream>
#include <vector>
#include <functional>
#include <cmath>
#include <glm/glm.hpp>
#include <iomanip>
#include <thread>
#include <boost/program_options.hpp>
#include <mutex>
#include "../include/utils.hpp"

namespace po = boost::program_options;

// Путь к ресурсам
const std::string res_path = "../res/";

const int buffer_size = 256;

std::mutex mutex;
std::vector<std::exception_ptr> exceptions;

/**
 * Удаление шума, нулевой дорожки и инвертирование сигнала
 * @param signal_file
 * @param noise_file
 * @param out_file
 */
void preprocess_signal(const std::string& signal_file,
                       const std::string& noise_file,
                       const std::string& out_file)
{
    const double low_border = 100;

    std::ifstream noise(noise_file);
    std::ifstream signal(signal_file);
    std::ofstream out(out_file);

    std::vector<std::string> lines;
    bool signal_appear = false;
    while (!(lines = sig::get_lines(signal, buffer_size)).empty()) {
        auto signal_lines = sig::parse_lines(lines);

        std::vector<glm::vec2> noise_lines =
                sig::parse_lines(sig::get_lines(noise, buffer_size));

        // Substract noise, inverse and shink
        for (size_t i = 0; i < signal_lines.size(); ++i) {
            signal_lines[i] -= noise_lines[i];
            signal_lines[i] *= -1;
        }

        if (!signal_appear) {
            for (auto it = signal_lines.begin(); it != signal_lines.end();) {
                if (std::abs((*it)[0]) > low_border) {
                    signal_appear = true;
                    break;
                } else {
                    it = signal_lines.erase(it);
                }
            }
        }

        for (const auto& line: signal_lines)
            out << std::fixed << std::setprecision(6)
                << line[0] << ";" << std::fixed << std::setprecision(6)
                << line[1]
                << "\n";
    }

    noise.close();
    signal.close();
    out.close();
}

/**
 * Медианный фильтр с окном в 3 элемента
 * @param lines
 * @return
 */
std::vector<glm::vec2> median_filter(const std::vector<glm::vec2>& lines)
{
    std::vector<glm::vec2> elements = lines;

    // Add extra elements
    elements.emplace(elements.begin(), *elements.begin());
    elements.push_back(elements.back());

    std::vector<glm::vec2> filtered;
    for (size_t i = 2; i < elements.size() - 1; ++i) {
        std::vector<glm::vec2> window = {elements[i - 1], elements[i],
                                         elements[i + 1]};
        window = sig::sort_pairs(window);
        filtered.push_back(window[1]); // Center element
    }

    return filtered;
}

/**
 * Обработка части сигнала
 * @param lines - строки с двумя каналами
 * @return
 */
std::vector<double> process_chunk(const std::vector<std::string>& lines,
                                  size_t lines_tot_count,
                                  glm::vec2 signal_min)
{
    auto signal_lines = sig::parse_lines(lines);

    // Инвертировать и нормировать x
    std::for_each(signal_lines.begin(), signal_lines.end(),
                  [signal_min](glm::vec2& line) {
                      line -= signal_min;
                  });

    // Нормировать по расстоянию
    sig::rescale(signal_lines, 0, lines_tot_count, 0, 12000);

    // Применение медианного фильтра
    // Добавлерие "лишних" элементов
    signal_lines.emplace(signal_lines.begin(), *signal_lines.begin());
    signal_lines.push_back(signal_lines.back());

    std::vector<glm::vec2> filtered = median_filter(signal_lines);

    std::vector<double> result;
    result.reserve(filtered.size());
    for (const auto& line: filtered)
        result.push_back(std::sqrt(std::pow(line[0], 2) + std::pow(line[1], 2)));

    return result;
}


/**
 *
 * @param signal_raw - Путь к "сырому" файлу
 * @param signal_file - Путь к промежуточному файлу
 * @param res_file  - Путь к файлу с конеченым результатом
 * @param noise_file - Путь к файлу с шумами
 */
void process_file(const std::string& signal_raw, const std::string& signal_file,
                  const std::string& res_file, const std::string& noise_file)
{
    try {
        // Удаление шума нулевой дорожки и  инвертирование
        preprocess_signal(signal_raw, noise_file, signal_file);

        std::ifstream signal(signal_file);
        std::ofstream result(res_file);
        if (!signal.good())
            throw std::runtime_error("Unable to open file: " + signal_file);
        else if (!result.good())
            throw std::runtime_error("Unable to open file: " + res_file);

        glm::vec4 min_max = sig::find_min_max(signal_file);
        glm::vec2 signal_min = glm::vec2(min_max[0], min_max[2]);
        glm::vec2 signal_max = glm::vec2(min_max[1], min_max[3]);
        std::vector<std::string> lines;
        size_t lines_count = sig::lines_count(signal_file);
        while (!(lines = sig::get_lines(signal, buffer_size)).empty()) {
            auto proccessed = process_chunk(lines, lines_count, signal_min);

            for (const auto &line: proccessed)
                result << std::fixed << std::setprecision(6) << line << "\n";
        }

        signal.close();
        result.close();
    } catch (...) {
        std::lock_guard<std::mutex> lock(mutex);
        exceptions.push_back(std::current_exception());
    }
}

int main(int argc, char* argv[])
{
    exceptions.clear();
    po::options_description options("Allowed options");
    options.add_options()
            ("help,h", "produce help message")
            ("signals,S", po::value<std::vector<std::string>>(), "Input .csv signal files")
            ("noise,N", po::value<std::string>(), "Input .csv noise file");

    po::variables_map vm;
    std::vector<std::string> signal_files;
    std::string noise_file;
    try {
        po::parsed_options parsed =
                po::command_line_parser(argc, argv).options(options).allow_unregistered().run();
        po::store(parsed, vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << options << "\n";
            return EXIT_SUCCESS;
        }

        if (!vm.count("signals") || !vm.count("noise")) {
            std::cout << "You need to provide signal and noise files\n";
            return EXIT_FAILURE;
        }

        signal_files = vm["signals"].as<std::vector<std::string>>();
        noise_file = vm["noise"].as<std::string>();
    } catch (std::exception& e) {
        std::cerr << "Unable to parse arguments\n";
        return EXIT_FAILURE;
    }

    std::vector<std::string> signal_raw = signal_files;
    std::vector<std::string> signal_path;
    for (size_t i = 0; i < signal_raw.size(); ++i) {
        signal_raw[i] = res_path + signal_raw[i];
        signal_path.push_back(res_path + "signal" + std::to_string(i) + ".csv");
    }

    std::string noise_path = res_path + noise_file;
    std::vector<std::string> result_path;
    for (size_t i = 0; i < signal_raw.size(); ++i)
        result_path.push_back(res_path + "res" + std::to_string(i) + ".csv");

    std::vector<std::thread> threads;
    // Запуск потоков
    for (size_t i = 0; i < signal_path.size(); ++i)
        threads.emplace_back(process_file, signal_raw[i], signal_path[i],
                             result_path[i], noise_path);

    for (auto& thr: threads)
        thr.join();

    // Т.к. исключения могут броситься в другом потоку
    // необходимо хранить их в отдельном месте
    for (auto& e: exceptions) {
        try {
            if (e)
                std::rethrow_exception(e);
        } catch (const std::exception& e) {
            std::cerr << "Error occured: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

    return 0;
}
