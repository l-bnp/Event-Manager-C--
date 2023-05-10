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
//  The on() method returns a size_t value that can be used to unregister the function from the event using the off() method like this:
//      size_t function_id = event_manager.on<int>("my_event", [](int value) { std::cout << "Received value: " << value << std::endl; });
//      event_manager.off("my_event", function_id);

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <functional>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <mutex>

// Define a template alias for std::function with variadic template arguments.
template <typename... Args>
using FunctionType = std::function<void(Args...)>;

// Define a template alias for a pair of a unique identifier (size_t) and a FunctionType.
template <typename... Args>
using FunctionIdPair = std::pair<size_t, FunctionType<Args...>>;

// Define a template alias for a vector of FunctionIdPair with variadic template arguments.
template <typename... Args>
using FunctionVector = std::vector<FunctionIdPair<Args...>>;

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
    // Static method to access the Singleton instance.
    static EventManager &getInstance()
    {
        static EventManager instance;
        return instance;
    }

    // Register a function or lambda function with a specific event name.
    template <typename... Args, typename F>
    size_t on(std::string eventName, F &&newFunc);

    template <typename... Args>
    void off(std::string eventName, size_t id);

    // Emit an event with the specified name and pass arguments to the registered functions.
    template <typename... Args>
    void emitEvent(std::string eventName, Args... args);

private:
    // Private constructor.
    EventManager() = default;

    // Delete copy constructor and copy assignment operator.
    EventManager(const EventManager &) = delete;
    EventManager &operator=(const EventManager &) = delete;
    // A map that associates event names with their respective function vectors.
    std::map<std::string, std::shared_ptr<BaseFunctionVector>> functionsMap;
    std::mutex functionsMapMutex;
};

// Register a function or lambda function with a specific event name.
template <typename... Args, typename F>
size_t EventManager::on(std::string eventName, F &&newFunc)
{
    std::lock_guard<std::mutex> lock(functionsMapMutex);
    FunctionType<Args...> func = newFunc;
    auto itr = functionsMap.find(eventName);
    size_t id = 0;

    // If the event name already exists, add the new function to the existing vector.
    if (itr != functionsMap.end())
    {
        auto &functions = static_cast<DerivedFunctionVector<Args...> *>(itr->second.get())->functions;
        id = functions.size();
        functions.emplace_back(id, func);
    }
    // If the event name does not exist, create a new vector and add the function to it.
    else
    {
        auto functionVector = std::make_shared<DerivedFunctionVector<Args...>>();
        functionVector->functions.emplace_back(id, func);
        functionsMap.insert({eventName, functionVector});
    }

    return id;
}

// Method to remove a function with a specific event name and function ID.
template <typename... Args>
void EventManager::off(std::string eventName, size_t id)
{
    std::lock_guard<std::mutex> lock(functionsMapMutex);
    auto itr = functionsMap.find(eventName);

    if (itr != functionsMap.end())
    {
        auto &functions = static_cast<DerivedFunctionVector<Args...> *>(itr->second.get())->functions;
        functions.erase(std::remove_if(functions.begin(), functions.end(),
                                       [&](const FunctionIdPair<Args...> &pair)
                                       { return pair.first == id; }),
                        functions.end());
    }
}

// Emit an event with the specified name and pass arguments to the registered functions.
template <typename... Args>
void EventManager::emitEvent(std::string eventName, Args... args)
{
    std::lock_guard<std::mutex> lock(functionsMapMutex);
    auto itr = functionsMap.find(eventName);

    // If the event name exists, execute all registered functions with the provided arguments.
    if (itr != functionsMap.end())
    {
        auto &functions = static_cast<DerivedFunctionVector<Args...> *>(itr->second.get())->functions;
        for (auto &funcPair : functions)
        {
            funcPair.second(args...);
        }
    }
}

#endif // EVENT_MANAGER_H