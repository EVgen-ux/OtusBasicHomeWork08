#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

#include <thread>
#include <future>  

#include "CRC32.hpp"
#include "IO.hpp"

std::atomic<bool> should_stop=false;


/// @brief Переписывает последние 4 байта значением value
void replaceLastFourBytes(std::vector<char> &data, uint32_t value) {
  std::copy_n(reinterpret_cast<const char *>(&value), 4, data.end() - 4);
}

/**
 * @brief Формирует новый вектор с тем же CRC32, добавляя в конец оригинального
 * строку injection и дополнительные 4 байта
 * @details При формировании нового вектора последние 4 байта не несут полезной
 * нагрузки и подбираются таким образом, чтобы CRC32 нового и оригинального
 * вектора совпадали
 * @param original оригинальный вектор
 * @param injection произвольная строка, которая будет добавлена после данных
 * оригинального вектора
 * @return новый вектор
 */

// std::atomic<bool> should_stop=false;

void hack(const std::vector<char> &original,
                       const std::string &injection, uint32_t minVal, uint32_t maxVal, const char *file) {

  std::thread::id this_id = std::this_thread::get_id();
 

  const uint32_t originalCrc32 = crc32(original.data(), original.size());

  std::vector<char> result(original.size() + injection.size() + 4);
  auto it = std::copy(original.begin(), original.end(), result.begin());
  std::copy(injection.begin(), injection.end(), it);

  /*
   * Внимание: код ниже крайне не оптимален.
   * В качестве доп. задания устраните избыточные вычисления
   */

  for (size_t i = minVal; i < maxVal; ++i) {
    // Заменяем последние четыре байта на значение i
    replaceLastFourBytes(result, uint32_t(i));
    // Вычисляем CRC32 текущего вектора result
    auto currentCrc32 = crc32(result.data(), result.size());

    if (currentCrc32 == originalCrc32) {
      std::cout << "Success\n";
      writeToFile(file, result);
      should_stop = true; // флаг остановки
    }
    // Отображаем прогресс
    if (i % 1000000 == 0) {
      std::cout << "thread id " << this_id;

      std::cout << "  progress: "
                << static_cast<double>(i) / static_cast<double>(maxVal)
                << std::endl;                     
    }

    if(should_stop) return;
    
  }

  throw std::logic_error("Can't hack");
}

int main(int argc, char **argv) {

  std::vector<std::thread> thr_vec;
  uint32_t n_threads =  std::thread::hardware_concurrency();

  if (argc != 3) {
    std::cerr << "Call with two args: " << argv[0]
              << " <input file> <output file>\n";
    return 1;
  }
const size_t maxVal = std::numeric_limits<uint32_t>::max();

  try {
    const std::vector<char> data = readFromFile(argv[1]);

  

    for (uint32_t i = 0; i < n_threads; i++) {

    thr_vec.emplace_back(std::thread {hack, data, "he-he-he", i*maxVal/n_threads, maxVal/n_threads + i*maxVal/n_threads, argv[2]});
    
   }

    for (auto& it : thr_vec) {
        it.join();
    }

  } catch (std::exception &ex) {
  //  std::cerr << ex.what() << '\n';
    return 2;
  }
  return 0;
}
