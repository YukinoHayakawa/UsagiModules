(R"(
std::pair<std::uint64_t, ResumeCondition>
script_main(std::uint64_t entry, {} &db)
{{
    // Invokes the template entry defined in the script main body.
    return script(entry, db);
}}
)" + 1 /* removes the newline at the beginning */)
