#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <termios.h>
#include <unistd.h>

namespace simpleprompt {
    class SimplePrompt
    {
      public:
        SimplePrompt(std::string const & commandDictionaryPath,
                     std::function<void(std::string const &)> const & comCallback,
                     std::string const & welcomeMessage = "",
                     std::string const & prompt = "prompt$> ")
          : m_commandDictionaryPath(commandDictionaryPath)
          , m_comCallback(comCallback)
          , m_welcomeMessage(welcomeMessage)
          , m_prompt(prompt)
          , m_commands()
        {
        }

        void addCommand(std::string const & com)
        {
            m_commands.push_back(com);
        }

        void start()
        {
             // disable echo and prevent need to press enter for getchar flush
            setupTerminal();
            loop();
        }

      private:
        /// An optional path to a file specifying a dictionary of commands.
        /// When empty, no path is specified.
        std::string m_commandDictionaryPath;

        /// Callback to run when a command is entered
        std::function<void(std::string const &)> m_comCallback;

        /// Prompt to display (e.g. ">>")
        std::string m_prompt;

        /// When prompt starts, display some welcome message
        std::string m_welcomeMessage;

        /// Command dictionary
        std::vector<std::string> m_commands;

        void setupTerminal()
        {
            static struct termios oldt, newt;

            // See http://stackoverflow.com/questions/1798511/how-to-avoid-press-enter-with-any-getchar
            // tcgetattr gets the parameters of the current terminal
            // STDIN_FILENO will tell tcgetattr that it should write the settings
            // of stdin to oldt
             
            tcgetattr( STDIN_FILENO, &oldt);
            // now the settings will be copied
            newt = oldt;

            
            // ICANON normally takes care that one line at a time will be processed
            // that means it will return if it sees a "\n" or an EOF or an EOL
            newt.c_lflag &= ~(ICANON);

            // also disable standard output -- Ben
            newt.c_lflag &= ~ECHO;
            newt.c_lflag |= ECHONL;
            newt.c_lflag &= ICRNL;

            // Those new settings will be set to STDIN
            // TCSANOW tells tcsetattr to change attributes immediately.
            tcsetattr( STDIN_FILENO, TCSANOW, &newt);
        }

        void removeLastChar()
        {
            // backspace which only moves cursor, so need to write a blank
            // space over the original char immediately afterwards
            // to emulate having removed it
            std::cout<<"\b ";
            // now move cursor back again
            std::cout<<"\b";
        }

        void handleBackspace(int &cursorPos, std::string &toReturn)
        {
            // check that the cursor position is > 0 to prevent accidental
            // deletion of prompt!
            if(cursorPos > 0) {
                removeLastChar();
                --cursorPos;
                std::string copy(std::begin(toReturn), std::end(toReturn) - 1);
                copy.swap(toReturn);
            }
        }

        /// attempts to tab-complete a command
        std::string tabCompleteCommand(std::string const &command)
        {
            // iterate over entries in folder
            for (auto const & it : m_commands) {

                // try to match the entry with the thing that we want to tab-complete
                auto extracted(it.substr(0, command.length()));
                if(extracted == command) {
                    return it; // match, return name of command
                }
            }
            return command; // no match, return un tab-completed version
        }

        /// handles tab-key; attempts to provide rudimentary tab-completion
        void handleTabKey(int &cursorPos,
                          std::string &toReturn)
        {
            // if the user hasn't entered anything, simply list the available commands
            if(toReturn.empty()) {
                return;
            }
            auto lenBefore = toReturn.length();
            toReturn = tabCompleteCommand(toReturn);

            // updated cursor position based on returned string
            cursorPos = toReturn.length();

            for(int i = 0;i < lenBefore; ++i) {
                removeLastChar();
            }

            // after effect of pressing tab, need to print out prompt
            // and where we were with toReturn again
            std::cout<<toReturn;
        }

        /// gets a user-inputted string until return is entered
        /// will use getChar to process characters one by one so that individual
        /// key handlers can be created
        std::string getInputString()
        {
            std::string toReturn("");

            int cursorPos(0);
            while(1) {
                char c = getchar();
                auto intCode = static_cast<int>(c);
                if(intCode == 10) { // enter
                    std::cout<<std::endl;
                    break;
                } else if(c == 3) { // ctrl-c, I think
                    exit(0);
                } else if(intCode == 65 || intCode == 66 ||
                   intCode == 67 || intCode == 68) { // arrow keys 
                    continue; // (i.e., ignore)
                } else if(intCode == 127 || intCode == 8) { // delete / backspace
                    handleBackspace(cursorPos, toReturn);
                } else if(intCode == 9) { // tab

                    handleTabKey(cursorPos, toReturn);

                } else { // print out char to screen and push into string vector
                    std::cout<<c;
                    ++cursorPos;
                    toReturn.push_back(c);
                }
            }
            return toReturn;
        }

        /// indefinitely loops over user input
        void loop()
        {
            std::string currentPath("/");
            while (1) {
                std::cout<<m_prompt;
                auto commandStr = getInputString();
            }
        }
    };
}