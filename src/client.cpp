#include "demo-adaptor-glue.h"
#include "demo-proxy-glue.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>

namespace
{
    constexpr const char *SERVICE_NAME = "com.example.DemoService";
    constexpr const char *OBJECT_PATH = "/com/example/Demo";
    constexpr const char *CALLBACK_OBJECT_PATH = "/com/example/DemoCallback";

    class DemoCallback final : public sdbus::AdaptorInterfaces<com::example::DemoCallback_adaptor>
    {
    public:
        DemoCallback(sdbus::IConnection &connection, sdbus::ObjectPath objectPath)
            : AdaptorInterfaces(connection, std::move(objectPath))
        {
            registerAdaptor();
        }

        ~DemoCallback()
        {
            unregisterAdaptor();
        }

    private:
        std::string onServerMessage(const std::string &message) override
        {
            const auto reply = "client callback received: " + message;
            std::cout << "callback onServerMessage: " << message << std::endl;
            return reply;
        }
    };

    class DemoClient final : public sdbus::ProxyInterfaces<com::example::Demo_proxy>
    {
    public:
        DemoClient(sdbus::IConnection &connection, sdbus::ServiceName destination, sdbus::ObjectPath objectPath)
            : ProxyInterfaces(connection, std::move(destination), std::move(objectPath))
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
        auto connection = sdbus::createSessionBusConnection();
        DemoCallback callback(*connection, sdbus::ObjectPath{CALLBACK_OBJECT_PATH});
        connection->enterEventLoopAsync();

        DemoClient client(*connection, sdbus::ServiceName{SERVICE_NAME}, sdbus::ObjectPath{OBJECT_PATH});

        const auto reply = client.echo(message);
        std::cout << "method echo reply: " << reply << std::endl;

        const auto sum = client.add(20, 22);
        std::cout << "method add reply : 20 + 22 = " << sum << std::endl;

        auto fd = client.openTempFile();
        const std::string fdMessage = "hello via fd\n";
        if (::write(fd.get(), fdMessage.data(), fdMessage.size()) < 0)
        {
            std::cerr << "fd write failed: " << std::strerror(errno) << std::endl;
            return 1;
        }

        if (::lseek(fd.get(), 0, SEEK_SET) < 0)
        {
            std::cerr << "fd seek failed: " << std::strerror(errno) << std::endl;
            return 1;
        }

        std::array<char, 256> buffer{};
        const auto bytesRead = ::read(fd.get(), buffer.data(), buffer.size() - 1);
        if (bytesRead < 0)
        {
            std::cerr << "fd read failed: " << std::strerror(errno) << std::endl;
            return 1;
        }

        std::cout << "method openTempFile fd content: " << buffer.data();

        client.registerCallback(sdbus::ObjectPath{CALLBACK_OBJECT_PATH});
        std::cout << "method registerCallback called; waiting for server callback..." << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }
    catch (const sdbus::Error &error)
    {
        std::cerr << "D-Bus error: " << error.getName() << ": " << error.getMessage() << std::endl;
        return 1;
    }

    return 0;
}
