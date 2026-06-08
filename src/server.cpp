#include "demo-server-glue.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <unistd.h>
#include <utility>

namespace
{
    constexpr const char *kServiceName = "com.example.DemoService";
    constexpr const char *kObjectPath = "/com/example/Demo";
    constexpr const char *kTempFilePath = "/tmp/sdbus-cpp-demo-fd.txt";

    class DemoService final : public sdbus::AdaptorInterfaces<com::example::Demo_adaptor>
    {
    public:
        DemoService(sdbus::IConnection &connection, sdbus::ObjectPath objectPath)
            : AdaptorInterfaces(connection, std::move(objectPath))
        {
            registerAdaptor();
        }

        ~DemoService()
        {
            unregisterAdaptor();
        }

    private:
        std::string echo(const std::string &message) override
        {
            ++echoCount_;

            const auto reply = "server received: " + message;
            std::cout << "echo(\"" << message << "\") -> \"" << reply << "\"" << std::endl;

            emitEchoed(message, echoCount_);
            return reply;
        }

        int32_t add(const int32_t &left, const int32_t &right) override
        {
            const auto sum = left + right;
            std::cout << "add(" << left << ", " << right << ") -> " << sum << std::endl;
            return sum;
        }

        sdbus::UnixFd openTempFile() override
        {
            const auto fd = ::open(kTempFilePath, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
            if (fd < 0)
            {
                throw sdbus::Error(sdbus::Error::Name{"com.example.Demo.Error.OpenFailed"}, std::strerror(errno));
            }

            std::cout << "openTempFile() -> " << kTempFilePath << " fd passed to client" << std::endl;
            return sdbus::UnixFd(fd, sdbus::adopt_fd);
        }

        uint32_t echoCount_{0};
    };
}

int main()
{
    try
    {
        auto connection = sdbus::createSessionBusConnection(sdbus::ServiceName{kServiceName});
        DemoService service(*connection, sdbus::ObjectPath{kObjectPath});

        std::cout << "D-Bus demo service is running." << std::endl;
        std::cout << "  service: " << kServiceName << std::endl;
        std::cout << "  object : " << kObjectPath << std::endl;

        connection->enterEventLoop();
    }
    catch (const sdbus::Error &error)
    {
        std::cerr << "D-Bus error: " << error.getName() << ": " << error.getMessage() << std::endl;
        return 1;
    }

    return 0;
}
