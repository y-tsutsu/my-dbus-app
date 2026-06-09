#include "demo-adaptor-glue.h"
#include "demo-proxy-glue.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <optional>
#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>

namespace
{
    constexpr const char *SERVICE_NAME = "com.example.DemoService";
    constexpr const char *OBJECT_PATH = "/com/example/Demo";
    constexpr const char *TEMP_FILE_PATH = "/tmp/sdbus-cpp-demo-fd.txt";

    class DemoCallbackClient final : public sdbus::ProxyInterfaces<com::example::DemoCallback_proxy>
    {
    public:
        DemoCallbackClient(sdbus::IConnection &connection, sdbus::ServiceName destination, sdbus::ObjectPath objectPath)
            : ProxyInterfaces(connection, std::move(destination), std::move(objectPath))
        {
            registerProxy();
        }

        ~DemoCallbackClient()
        {
            unregisterProxy();
        }
    };

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
        struct CallbackRegistration
        {
            std::string serviceName;
            std::string objectPath;
        };

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
            const auto fd = ::open(TEMP_FILE_PATH, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
            if (fd < 0)
            {
                throw sdbus::Error(sdbus::Error::Name{"com.example.Demo.Error.OpenFailed"}, std::strerror(errno));
            }

            std::cout << "openTempFile() -> " << TEMP_FILE_PATH << " fd passed to client" << std::endl;
            return sdbus::UnixFd(fd, sdbus::adopt_fd);
        }

        void registerCallback(const sdbus::ObjectPath &callbackPath) override
        {
            const auto sender = getObject().getCurrentlyProcessedMessage().getSender();
            if (sender == nullptr || sender[0] == '\0')
            {
                throw sdbus::Error(sdbus::Error::Name{"com.example.Demo.Error.MissingSender"}, "callback sender is missing");
            }

            callback_ = CallbackRegistration{sender, callbackPath};
            std::cout << "registerCallback(sender=" << callback_->serviceName << ", path=" << callback_->objectPath << ")" << std::endl;

            callCallbackLater(*callback_);
        }

        void callCallbackLater(CallbackRegistration callback)
        {
            std::thread([this, callback = std::move(callback)]
                        {
                std::this_thread::sleep_for(std::chrono::milliseconds{200});

                DemoCallbackClient callbackClient(getObject().getConnection(), sdbus::ServiceName{callback.serviceName}, sdbus::ObjectPath{callback.objectPath});
                const auto reply = callbackClient.onServerMessage("hello from server callback");

                std::cout << "callback reply: " << reply << std::endl; })
                .detach();
        }

        std::optional<CallbackRegistration> callback_;
        uint32_t echoCount_{0};
    };
}

int main()
{
    try
    {
        auto connection = sdbus::createSessionBusConnection(sdbus::ServiceName{SERVICE_NAME});
        DemoService service(*connection, sdbus::ObjectPath{OBJECT_PATH});

        std::cout << "D-Bus demo service is running." << std::endl;
        std::cout << "  service: " << SERVICE_NAME << std::endl;
        std::cout << "  object : " << OBJECT_PATH << std::endl;

        connection->enterEventLoop();
    }
    catch (const sdbus::Error &error)
    {
        std::cerr << "D-Bus error: " << error.getName() << ": " << error.getMessage() << std::endl;
        return 1;
    }

    return 0;
}
