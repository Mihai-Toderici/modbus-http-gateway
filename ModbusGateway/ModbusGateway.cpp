#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

#include <modbus/modbus.h>
#include <httplib.h>

//variabila globala pt registrii ca sa fie sincronizati intre server-ul de modbus si cel de http
std::atomic<uint16_t> holding_registers[10];


void run_http_server() {
    httplib::Server svr;

    // ex: GET /api/register/0
    svr.Get("/api/register/0", [](const httplib::Request& req, httplib::Response& res) {
        uint16_t val = holding_registers[0].load();
        res.set_content(std::to_string(val), "text/plain");
        });

    // ex: POST /api/register/0?value=123
    svr.Post("/api/register/0", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("value")) {
            res.set_content("Missing 'value' parameter", "text/plain");
            res.status = 400; // Bad Request
            return;
        }

        try {
            int val_from_req = std::stoi(req.get_param_value("value"));
            if (val_from_req >= 0 && val_from_req <= 65535) {
                holding_registers[0].store(static_cast<uint16_t>(val_from_req));
                res.set_content("OK", "text/plain");
            }
            else {
                res.set_content("Value out of range (0-65535)", "text/plain");
                res.status = 400;
            }
        }
        catch (const std::exception&) {
            res.set_content("Invalid 'value' parameter", "text/plain");
            res.status = 400;
        }
        });

    std::cout << "HTTP server starting on http://localhost:8080 ..." << std::endl;
    svr.listen("0.0.0.0", 8080);
}

void run_modbus_server() {
    modbus_t* ctx = nullptr;
    modbus_mapping_t* mb_mapping = nullptr;

    //ascultam toate interfatele de net
    ctx = modbus_new_tcp("0.0.0.0", 502);
    if (ctx == nullptr) {
        std::cerr << "Failed to create Modbus context: " << modbus_strerror(errno) << std::endl;
        return;
    }
    // modbus_set_debug(ctx, TRUE); // pt debugging

    //alocam memorie pt 10 registrii
    mb_mapping = modbus_mapping_new(0, 0, 10, 0);
    if (mb_mapping == nullptr) {
        std::cerr << "Failed to create Modbus mapping: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return;
    }

    int server_socket = modbus_tcp_listen(ctx, 1);
    if (server_socket == -1) {
        std::cerr << "Modbus listen error: " << modbus_strerror(errno) << std::endl;
        modbus_mapping_free(mb_mapping);
        modbus_free(ctx);
        return;
    }
    std::cout << "Modbus server is listening on port 502..." << std::endl;

    std::vector<uint8_t> query(MODBUS_TCP_MAX_ADU_LENGTH);

    while (true) {
        if (modbus_tcp_accept(ctx, &server_socket) == -1) {
            std::cerr << "Modbus accept error: " << modbus_strerror(errno) << std::endl;
            continue;
        }
        std::cout << "Modbus client connected." << std::endl;

        while (true) {

            for (int i = 0; i < 10; ++i) {
                mb_mapping->tab_registers[i] = holding_registers[i].load();
            }

            int rc = modbus_receive(ctx, query.data());
            if (rc == -1) {

                std::cerr << "Modbus client disconnected." << std::endl;
                break;
            }

           //Ne ocupam de request-uri
            rc = modbus_reply(ctx, query.data(), rc, mb_mapping);
            if (rc == -1) {
                std::cerr << "Modbus reply error: " << modbus_strerror(errno) << std::endl;
                break;
            }

          //Sincronizare
            for (int i = 0; i < 10; ++i) {
                holding_registers[i].store(mb_mapping->tab_registers[i]);
            }
        }
    }
    //Curatenie
    modbus_mapping_free(mb_mapping);
    modbus_free(ctx);
}

int main() {

    std::thread http_thread(run_http_server);

    std::thread modbus_thread(run_modbus_server);

    std::cout << "\n--- Modbus/HTTP Gateway is RUNNING ---" << std::endl;
    std::cout << "Press Enter to exit." << std::endl;
    std::cin.get();

    return 0;
}