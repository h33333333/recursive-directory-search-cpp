#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

std::vector<std::string> RESULTS;
std::mutex RESULTS_MUTEX;

int search_in_path(const std::string &path, const char *query) {
  if (std::filesystem::is_directory(path)) {
    std::vector<std::thread> threads;
    std::filesystem::directory_iterator iterator;
    try {
      iterator = std::filesystem::directory_iterator(path);
    } catch (const std::__1::__fs::filesystem::filesystem_error &err) {
      if (err.code().value() == 13) {
        std::cout << "Permission denied: " << path << std::endl;
      } else if (err.code().value() == 1) {
        std::cout << "Operation not permitted: " << path << std::endl;
      } else {
        std::cout << "Unexpected filesystem_error during directory iteration: "
                  << err.what() << std::endl;
      }
      return 1;
    } catch (...) {
      std::cout << "Unexpected error during directory iteration" << std::endl;
      return 1;
    }
    for (const auto &entry : iterator) {
      std::thread thread;
      while (true) {
        try {
          thread = std::thread(search_in_path, entry.path(), query);
          break;
        } catch (const std::__1::system_error &err) {
          // This error happens when we reach max amount of threads
          if (err.code().value() == 35) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          } else {
            std::cout << "Unexpected system_error while creating new thread: "
                      << err.what() << std::endl;
            return 1;
          }

        } catch (...) {
          std::cout << "Unexpected error while creating new thread"
                    << std::endl;
          return 1;
        }
      }
      threads.emplace_back(std::move(thread));
    }
    for (auto &thread : threads) {
      thread.join();
    }
    return 0;
  } else {
    std::ifstream file;
    file.open(path);
    if (!file.is_open()) {
      std::cout << "Error while opening file: " << path << std::endl;
      return 1;
    } else {
      std::string line;
      file >> line;
      while (!file.eof()) {
        if (line.find(query) != std::string::npos) {
          RESULTS_MUTEX.lock();
          RESULTS.emplace_back(path);
          RESULTS_MUTEX.unlock();
          break;
        }
        file >> line;
      }
      file.close();
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout
        << "Not enough arguments. Usage: ./recursive-directory-search <PATH> <QUERY>"
        << std::endl;
    return 1;
  }
  const std::string path(argv[1]);
  if (!std::filesystem::exists(path)) {
    std::cout << "Given path is not valid" << std::endl;
    return 1;
  }
  const char *query = argv[2];
  search_in_path(path, query);
  for (auto &result : RESULTS) {
    std::cout << result << std::endl;
  }
  return 0;
}
