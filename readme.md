- Unordered map to stored built-in commands and their respective funtion pointers
    - `std::unordered_map<std::string, void (*)(const std::vector<std::string>&)> builtinCommands`

- execute command function 
    - if builtin command then execute 
    - else fork the process and execvp it
    - else failed to fork
