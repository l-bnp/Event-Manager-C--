// event_manager.h

// The EventManager class allows you to register functions or lambda functions to event names,
// and emit these events, executing all registered functions for a given event.
// The functions in each vector can have different signatures but the functions in the same function vector must have the same signature.
//
// Example usage:
//     EventManager event_manager;
//
//     event_manager.on<int>("my_event", [](int value) { std::cout << "Received value: " << value << std::endl; });
//     event_manager.emitEvent<int>("my_event", 42);
//
//     event_manager.on<const std::string &, unsigned int, int>("set_volume", [this](const std::string &channel_type, unsigned int channel_number, int volume_db)
//                                                             { this->setVolume(channel_type, channel_number, volume_db); });
//     event_manager.emitEvent(commandJson.at("command_type").get<std::string>(), commandJson.at("channel_type").get<std::string>(),
//                                            commandJson.at("channel_number").get<unsigned int>(), commandJson.at("volume_db").get<int>());
//
// This is a minimal implementation. the usual off() method to unregister functions can be easily implemented.

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <functional>
#include <vector>
#include <map>
#include <memory>
#include <string>

// Define a template alias for std::function with variadic template arguments.
template <typename... Args>
using FunctionType = std::function<void(Args...)>;

// Define a template alias for a vector of FunctionType with variadic template arguments.
template <typename... Args>
using FunctionVector = std::vector<FunctionType<Args...>>;

// Base class for a vector of functions. It will be inherited by DerivedFunctionVector.
struct BaseFunctionVector
{
    virtual ~BaseFunctionVector() = default;
};

// Derived class template for holding a vector of functions with specific argument types.
template <typename... Args>
struct DerivedFunctionVector : public BaseFunctionVector
{
    FunctionVector<Args...> functions;
};

class EventManager
{
public:
    // Register a function or lambda function with a specific event name.
    template <typename... Args, typename F>
    void on(std::string eventName, F &&newFunc);

    // Emit an event with the specified name and pass arguments to the registered functions.
    template <typename... Args>
    void emitEvent(std::string eventName, Args... args);

private:
    // A map that associates event names with their respective function vectors.
    std::map<std::string, std::shared_ptr<BaseFunctionVector>> functionsMap;
};

// Register a function or lambda function with a specific event name.
template <typename... Args, typename F>
void EventManager::on(std::string eventName, F &&newFunc)
{
    FunctionType<Args...> func = newFunc;
    auto itr = functionsMap.find(eventName);

    // If the event name already exists, add the new function to the existing vector.
    if (itr != functionsMap.end())
    {
        static_cast<DerivedFunctionVector<Args...> *>(itr->second.get())->functions.push_back(func);
    }
    // If the event name does not exist, create a new vector and add the function to it.
    else
    {
        auto functionVector = std::make_shared<DerivedFunctionVector<Args...>>();
        functionVector->functions.push_back(func);
        functionsMap.insert({eventName, functionVector});
    }
}

// Emit an event with the specified name and pass arguments to the registered functions.
template <typename... Args>
void EventManager::emitEvent(std::string eventName, Args... args)
{
    auto itr = functionsMap.find(eventName);

    // If the event name exists, execute all registered functions with the provided arguments.
    if (itr != functionsMap.end())
    {
        auto &functions = static_cast<DerivedFunctionVector<Args...> *>(itr->second.get())->functions;
        for (auto &func : functions)
        {
            func(args...);
        }
    }
}

#endif // EVENT_MANAGER_H