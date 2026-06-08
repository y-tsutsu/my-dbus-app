#include "demo-client-glue.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <thread>
#include <utility>

namespace
{
    constexpr const char *kServiceName = "com.example.DemoService";
    constexpr const char *kObjectPath = "/com/example/Demo";

    class DemoClient final : public sdbus::ProxyInterfaces<com::example::Demo_proxy>
    {
    public:
        DemoClient(sdbus::ServiceName destination, sdbus::ObjectPath objectPath)
            : ProxyInterfaces(std::move(destination), std::move(objectPath))
        {
            registerProxy();
        }

        ~DemoClient()
        {
            unregisterProxy();
        }

    private:
        void onEchoed(const std::string &message, const uint32_t &callCount) override
        {
            std::cout << "signal echoed: message=\"" << message << "\", callCount=" << callCount << std::endl;
        }
    };
}

int main(int argc, char **argv)
{
    const std::string message = argc >= 2 ? argv[1] : "hello from client";

    try
    {
        DemoClient client(sdbus::ServiceName{kServiceName}, sdbus::ObjectPath{kObjectPath});

        const auto reply = client.echo(message);
        std::cout << "method echo reply: " << reply << std::endl;

        const auto sum = client.add(20, 22);
        std::cout << "method add reply : 20 + 22 = " << sum << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }
    catch (const sdbus::Error &error)
    {
        std::cerr << "D-Bus error: " << error.getName() << ": " << error.getMessage() << std::endl;
        return 1;
    }

    return 0;
}
