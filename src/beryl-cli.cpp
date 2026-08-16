// © Arya Irawan — 10 August 2026

#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

static const int RPC_PORT = 18444;

int main(
    int argc,
    char* argv[]
)
{
    if(argc < 2)
    {
        std::cout
            << "usage: beryl-cli command\n"
            << "commands:\n"
            << "  getseedphrase\n"
            << "  restorewallet <24-word-seed>\n"
            << "  getbalance\n"
            << "  getwalletinfo\n"
            << "  getnewaddress\n"
            << "  getaddresses\n"
            << "  getblockcount\n"
            << "  getblock <height>\n"
            << "  getdifficulty\n"
            << "  getmininginfo\n"
            << "  gettransaction <txid>\n"
            << "  sendtoaddress <address> <amount>\n";

        return 0;
    }

    // ========================================================
    // BUILD RPC COMMAND
    // Gabungkan seluruh argument setelah nama command.
    //
    // Contoh:
    //   beryl-cli sendtoaddress berxxxx 1
    //
    // menjadi:
    //   "sendtoaddress berxxxx 1"
    // ========================================================

    // Gabungkan seluruh argumen menjadi satu command RPC.
    // Gabungkan seluruh argumen menjadi satu command RPC.
    // Contoh:
    // restorewallet <24 kata>
    // sendtoaddress <address> <amount>
    // gettransaction <txid>

    std::string command = argv[1];

    for (int i = 2; i < argc; ++i)
    {
        command += " ";
        command += argv[i];
    }

    std::string request =
        command + "\n";

    // ========================================================
    // CONNECT TO BERYL RPC SERVER
    // ========================================================

    int sock = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (sock < 0)
    {
        std::cerr
            << "ERROR: cannot create RPC socket\n";

        return 1;
    }

    sockaddr_in server{};

    server.sin_family =
        AF_INET;

    server.sin_port =
        htons(RPC_PORT);

    if (inet_pton(
            AF_INET,
            "127.0.0.1",
            &server.sin_addr
        ) <= 0)
    {
        std::cerr
            << "ERROR: invalid RPC address\n";

        close(sock);

        return 1;
    }

    if (connect(
            sock,
            reinterpret_cast<sockaddr*>(&server),
            sizeof(server)
        ) < 0)
    {
        std::cerr
            << "Beryl daemon belum aktif\n";

        close(sock);

        return 1;
    }


    // --------------------------------------------------------
    // Kirim SELURUH RPC request.
    // TCP adalah stream, send() tidak menjamin seluruh
    // buffer terkirim dalam satu kali pemanggilan.
    // --------------------------------------------------------
    size_t sent = 0;

    while (sent < request.size())
    {
        const ssize_t n = send(
            sock,
            request.data() + sent,
            request.size() - sent,
            0
        );

        if (n <= 0)
        {
            std::cerr
                << "ERROR: RPC request failed\n";

            close(sock);
            return 1;
        }

        sent += static_cast<size_t>(n);
    }

    // --------------------------------------------------------
    // Request sudah selesai dikirim.
    // Server RPC membaca sampai EOF.
    // Socket tetap terbuka untuk menerima response.
    // --------------------------------------------------------
    shutdown(sock, SHUT_WR);

    char buffer[4096];

    while(true)
    {
        ssize_t received = recv(
            sock,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if(received <= 0)
            break;

        buffer[received] = '\0';

        std::cout << buffer;

        if(received < static_cast<ssize_t>(
            sizeof(buffer) - 1
        ))
        {
            break;
        }
    }

    close(sock);

    return 0;
}
