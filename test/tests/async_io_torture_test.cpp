#include "test_functions.hpp"

BOOST_AFIO_AUTO_TEST_CASE(async_io_torture, "Tortures the async i/o implementation", 120)
{
    using namespace BOOST_AFIO_V2_NAMESPACE;
    namespace asio = BOOST_AFIO_V2_NAMESPACE::asio;
    auto dispatcher = make_dispatcher(process_threadpool(), file_flags::none);
    std::cout << "\n\nSustained random i/o to 10 files of 10Mb:\n";
    evil_random_io(dispatcher, 10, 10 * 1024 * 1024);
}