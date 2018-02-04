#include "SimplePrompt.hpp"

int main()
{
    simpleprompt::SimplePrompt sp("",[](std::string const&){});
    sp.addCommand("remove");
    sp.addCommand("mkdir");
    sp.start();
    return 0;
}