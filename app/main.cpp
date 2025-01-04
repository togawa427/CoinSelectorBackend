#include "httplib.h"
#include <unistd.h>

int main(void)
{
    using namespace httplib;
    Server svr;

    std::cout << "start Server" << std::endl;
    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello World!!!", "text/plain");
    });

    svr.Get(R"(/numbers/(\d+))", [&](const Request& req, Response& res) {
        auto numbers = req.matches[1];
        res.set_content(numbers, "text/plain");
    });

    svr.listen("localhost", 1234);
}