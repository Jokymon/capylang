#include <iostream>
#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messagehandler.h>
#include <lsp/messages.h>
#include <lsp/process.h>

template <typename MessageType>
void printMessageMethod()
{
    const auto type = lsp::message::IsNotification<MessageType> ? "notification" : "request";
    std::cerr << "Server received " << type << " '" << MessageType::Method << '\'' << std::endl;
}

template <typename MessageType>
void printMessagePayload(const typename MessageType::Params& params)
{
    const auto json = lsp::toJson(typename MessageType::Params(params));
    std::cerr << "payload: " << lsp::json::stringify(json, true) << std::endl;
}

template <typename MessageType>
void printMessage(const typename MessageType::Params& params)
{
    printMessageMethod<MessageType>();
    printMessagePayload<MessageType>(params);
}

template <typename MessageType>
void printMessage()
{
    printMessageMethod<MessageType>();
}

class LanguageServer
{
public:
    LanguageServer(lsp::io::Stream& io)
    : m_connection{io}
    , m_messageHandler{m_connection}
    {
        registerCallbacks();
        m_state.store(State::Uninitialized);
    }

    bool isRunning()
    {
#ifdef LSP_PROCESS_UNSUPPORTED
        const auto parentAlive = true;
#else
        const auto parentAlive =
            m_parentProcessId.isNull() || lsp::Process::exists(*m_parentProcessId);
#endif
        return (m_state.load() != State::Inactive) && parentAlive;
    }

    int run()
    {
        while (isRunning())
            m_messageHandler.processIncomingMessages();

        return EXIT_SUCCESS;
    }

    auto initialize(lsp::InitializeParams&& params) -> lsp::RequestResult<lsp::requests::Initialize>
    {
        std::cerr << "INITIALIZE" << std::endl;
        printMessage<lsp::requests::Initialize>(params);
        if (m_state > State::Uninitialized)
        {
            return std::unexpected(
                lsp::RequestError(lsp::MessageError::InvalidRequest, "Already initialized")
            );
        }

        m_state.store(State::Active);
        m_parentProcessId = params.processId;

        /*
         * Respond with an InitializeResult containing some basic server info and capabilities
         */

        return lsp::InitializeResult{
            .capabilities = {
                .positionEncoding = lsp::PositionEncodingKind::UTF16,
                .textDocumentSync = lsp::TextDocumentSyncOptions{
                    .openClose = true,
                    .change = lsp::TextDocumentSyncKind::Full,
                    .save = true
                },
                .hoverProvider = true,
            },
            .serverInfo = lsp::InitializeResultServerInfo{.name = "Language Server Example", .version = "1.0.0"},
        };
    }

    lsp::NotificationResult textDocumentDidOpen(lsp::notifications::TextDocument_DidOpen::Params&& params)
    {
        if (auto res = verifyInitialized(); !res.has_value())
            return res;

        // TODO: Do something with the openend document here...
        (void)params;
        return {};
    }

    auto hover(lsp::requests::TextDocument_Hover::Params&& params) -> lsp::RequestResult<lsp::requests::TextDocument_Hover>
    {
        if (auto res = verifyInitialized(); !res.has_value())
            return std::unexpected(res.error());

        std::string hover_text = std::string("Hovering over file ") +
                                 params.textDocument.uri.path() + " at line " +
                                 std::to_string(params.position.line) + ", char pos " +
                                 std::to_string(params.position.character);

        // return the result
        // TextDocument_Hover::Result is NullOr<Hover>
        auto hover = lsp::Hover{
            .contents = lsp::MarkupContent{
                .kind = lsp::MarkupKind::PlainText,
                .value = hover_text
            }
        };
        return lsp::requests::TextDocument_Hover::Result(std::move(hover));
    }

    auto shutdown() -> lsp::requests::Shutdown::Result
    {
        printMessage<lsp::requests::Shutdown>();
        m_state.store(State::Shutdown);
        return {};
    }

    void exit()
    {
        printMessage<lsp::notifications::Exit>();
        m_state.store(State::Inactive);
    }

private:
    lsp::Connection m_connection;
    lsp::MessageHandler m_messageHandler;
    lsp::NullOr<int> m_parentProcessId;

    enum class State
    {
        Inactive,      // Initial state or exit notification received
        Uninitialized, // Started up and waiting for initialize request
        Active,        // Currently handling requests
        Shutdown       // Shutdown notification received
    };

    std::atomic<State> m_state = State::Uninitialized;

    lsp::NotificationResult verifyInitialized() const
    {
        if (m_state.load() <= State::Uninitialized)
        {
            return std::unexpected(
                lsp::RequestError(lsp::MessageError::ServerNotInitialized, "Server not initialized")
            );
        }

        if (m_state.load() == State::Shutdown)
        {
            return std::unexpected(
                lsp::RequestError(lsp::MessageError::InvalidRequest, "Shutdown request received")
            );
        }

        return {};
    }

    void registerCallbacks()
    {
        m_messageHandler.add<lsp::requests::Initialize>(
                            [this](lsp::requests::Initialize::Params&& params)
                            {
                                return initialize(std::move(params));
                            }
        ).add<lsp::notifications::TextDocument_DidOpen>([this](lsp::notifications::TextDocument_DidOpen::Params&& params)
                                                        { return textDocumentDidOpen(std::move(params)); })
            .add<lsp::requests::TextDocument_Hover>([this](lsp::requests::TextDocument_Hover::Params&& params)
                                                    { return hover(std::move(params)); })
            .add<lsp::requests::Shutdown>([this]()
                                          { return shutdown(); })
            .add<lsp::notifications::Exit>([this]()
                                           { exit(); });
    }
};

int runStdioServer()
{
    auto server = LanguageServer(lsp::io::standardIO());
    return server.run();
}

int main(int argc, char* argv[])
{
    std::cerr << "Starting stdio server\n";

    return runStdioServer();
}
