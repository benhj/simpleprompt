#include "SimplePrompt.hpp"
#include <iostream>
#include <string>

int main()
{
    auto const printer = [](std::string const & str) {
        std::cout<<str;
    };
    simpleprompt::SimplePrompt sp("",[](std::string const&){}, printer);
    sp.addCommand("remove");
    sp.addCommand("mkdir");
    sp.start();
    return 0;
}