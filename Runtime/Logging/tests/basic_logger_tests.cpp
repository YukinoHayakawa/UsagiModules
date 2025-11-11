#define BOOST_TEST_MODULE UsagiModules_Logging_BasicLoggerTests

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/test/included/unit_test.hpp>

#include <Usagi/Modules/Runtime/Logging/RuntimeLogger.hpp>

// Helper function to read a file into a string
std::string read_log_file(const std::string & filename)
{
    std::ifstream log_file(filename);
    if(!log_file.good())
    {
        return "";
    }
    std::stringstream file_content;
    file_content << log_file.rdbuf();
    return file_content.str();
}

BOOST_AUTO_TEST_CASE(LogsTestOutputString)
{
    const std::string filename = "log_test_output_string.txt";
    std::remove(filename.c_str());

    {
        usagi::runtime::RuntimeLogger logger("TestLogger");
        logger.add_file_sink(filename);
        logger.info("<test_output_string>");
    }

    const auto output = read_log_file(filename);
    std::remove(filename.c_str());

    BOOST_CHECK(output.find("<test_output_string>") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(LogLevels)
{
    const std::string filename = "log_levels.txt";
    std::remove(filename.c_str());

    {
        usagi::runtime::RuntimeLogger logger("TestLogger");
        logger.add_file_sink(filename);

        logger.trace("trace message");
        logger.debug("debug message");
        logger.info("info message");
        logger.warn("warn message");
        logger.error("error message");
        logger.critical("critical message");
    }

    const auto output = read_log_file(filename);
    std::remove(filename.c_str());

    BOOST_CHECK(output.find("trace message") != std::string::npos);
    BOOST_CHECK(output.find("[trace]") != std::string::npos);

    BOOST_CHECK(output.find("debug message") != std::string::npos);
    BOOST_CHECK(output.find("[debug]") != std::string::npos);

    BOOST_CHECK(output.find("info message") != std::string::npos);
    BOOST_CHECK(output.find("[info]") != std::string::npos);

    BOOST_CHECK(output.find("warn message") != std::string::npos);
    BOOST_CHECK(output.find("[warning]") != std::string::npos);

    BOOST_CHECK(output.find("error message") != std::string::npos);
    BOOST_CHECK(output.find("[error]") != std::string::npos);

    BOOST_CHECK(output.find("critical message") != std::string::npos);
    BOOST_CHECK(output.find("[critical]") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(LogDifferentDataTypes)
{
    const std::string filename = "log_data_types.txt";
    std::remove(filename.c_str());

    {
        usagi::runtime::RuntimeLogger logger("TestLogger");
        logger.add_file_sink(filename);

        logger.info("Logging an integer: {}", 42);
        logger.info("Logging a float: {}", 3.14f);
        logger.info("Logging a string: {}", "hello");
    }

    const auto output = read_log_file(filename);
    std::remove(filename.c_str());

    BOOST_CHECK(output.find("Logging an integer: 42") != std::string::npos);
    BOOST_CHECK(output.find("Logging a float: 3.14") != std::string::npos);
    BOOST_CHECK(output.find("Logging a string: hello") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(NoSinkNoOutput)
{
    const std::string filename = "log_no_sink.txt";
    std::remove(filename.c_str());

    {
        usagi::runtime::RuntimeLogger logger("TestLogger");
        // No sink is added.
        logger.info("This should not be logged");
    }

    // Check that the file was not created.
    std::ifstream log_file(filename);
    BOOST_CHECK(!log_file.good());
}

// This test is redundant now as all tests use a file sink,
// but we keep it to explicitly verify the file sink functionality.
BOOST_AUTO_TEST_CASE(FileSink)
{
    const std::string filename = "log_file_sink.txt";
    std::remove(filename.c_str());
    {
        usagi::runtime::RuntimeLogger logger("FileTestLogger");
        logger.add_file_sink(filename);
        logger.info("This is a file log message.");
    }

    const auto output = read_log_file(filename);
    std::remove(filename.c_str());

    BOOST_CHECK(
        output.find("This is a file log message.") != std::string::npos
    );
    // The init message is not logged as the sink is added after construction.
    // The logger name is not in the default pattern.
    // So we can't check for the logger name here.
}
