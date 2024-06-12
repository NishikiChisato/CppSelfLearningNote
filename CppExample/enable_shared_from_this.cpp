#include <functional>
#include <iostream>
#include <memory>
#include <thread>

/**
 * @brief If you want to let your call to gain ability to create
 * std::shared_ptr<class name> instances from within member function, you should
 * use std::enable_shared_from_this
 *
 * It has two benefits:
 * - Avoid multiple ownership: if you directly pass this pointer to function
 * that expects a std::shared_ptr, it meight end up with multiple
 * std::shared_ptr instances managing the same raw pointer, leading to multiple
 * deletion
 *
 * - Ensure objective's lifetime: as the following example shown, if object of
 * Worker go out of scope or is deleted before callback function execute, it
 * will result in UB(undefined behavior), since this object pointed by this
 * pointer is deleted
 *
 */

class GoodWork : public std::enable_shared_from_this<GoodWork> {
public:
  GoodWork() { std::cout << "Construct" << std::endl; }
  ~GoodWork() {
    std::cout << "Deconstruct" << std::endl;
    x = 0;
  }
  void doWork() {
    std::thread t([self = shared_from_this()]() {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "Doing Work" << std::endl;
      auto callback = std::bind(&GoodWork::callbackFunc, self);
      callback();
    });
    t.detach();
  }
  void callbackFunc() {
    std::cout << "Callback Func" << std::endl;
    std::cout << "x = " << x << std::endl;
  }

  // x initialize to 10, the destructor will change its value to 0, means this
  // variable is destroied
  int x = 10;
};

class BadWork {
public:
  BadWork() { std::cout << "Construct" << std::endl; }
  ~BadWork() {
    std::cout << "Deconstruct" << std::endl;
    x = 0;
  }
  void doWork() {
    std::thread t([self = this]() {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "Doing Work" << std::endl;
      auto callback = std::bind(&BadWork::callbackFunc, self);
      callback();
    });
    t.detach();
  }
  void callbackFunc() {
    std::cout << "Callback Func" << std::endl;
    std::cout << "x = " << x << std::endl;
  }

  // x initialize to 10, the destructor will change its value to 0, means this
  // variable is destroied
  int x = 10;
};

int main(int argc, char *argv[], char *envp[]) {
  {
    auto ptr = std::make_shared<GoodWork>();
    ptr->doWork();
  }
  std::cout << "obj has destoried" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // {
  //   auto ptr = std::make_shared<BadWork>();
  //   ptr->doWork();
  // }
  // std::cout << "obj has destoried" << std::endl;
  // std::this_thread::sleep_for(std::chrono::seconds(1));
  return 0;
}