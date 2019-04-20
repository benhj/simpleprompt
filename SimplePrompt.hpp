// This 'ere software is hacky as fuck. Use at your peril.
// I'd appreciate a mention if you do. But if you don't,
// I'm not going to lose sleep over it.
//
// Ben H.D. Jones in the year of 2018-2019

#pragma once

#include <deque>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <termios.h>
#include <unistd.h>

#define SP_UP_ARROW 65
#define SP_DOWN_ARROW 66
#define SP_LEFT_ARROW 68
#define SP_RIGHT_ARROW 67
#define SP_CTRL_C 3
#define SP_CTRL_E 5
#define SP_CTRL_A 1
#define SP_CTRL_K 11
#define SP_ENTER 10
#define SP_BACKSPACE 127
#define SP_DELETE 8
#define SP_TAB 9

namespace simpleprompt {
    class SimplePrompt
    {
      public:
        SimplePrompt(std::string const & commandDictionaryPath,
                     std::function<void(std::string const &)> const & comCallback,
                     std::function<void(std::string const &)> const & printer,
                     std::string const & welcomeMessage = "",
                     std::string const & prompt = "prompt$> ")
          : m_commandDictionaryPath(commandDictionaryPath)
          , m_comCallback(comCallback)
          , m_printer(printer)
          , m_welcomeMessage(welcomeMessage)
          , m_prompt(prompt)
          , m_commands()
          , m_history()
        {
            if(!m_welcomeMessage.empty()) {
                std::cout<<"\n\n"<<welcomeMessage<<std::endl;
                std::cout<<"\n\n"<<std::endl;
            }
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

        ~SimplePrompt()
        {
            // Revert terminal state
            tcsetattr( STDIN_FILENO, TCSANOW, &m_oldt);
        }

      private:
        /// An optional path to a file specifying a dictionary of commands.
        /// When empty, no path is specified.
        std::string m_commandDictionaryPath;

        /// Callback to run when a command is entered
        std::function<void(std::string const &)> m_comCallback;

        /// Callback to handle printing
        std::function<void(std::string const &)> m_printer;

        /// Prompt to display (e.g. ">>")
        std::string m_prompt;

        /// When prompt starts, display some welcome message
        std::string m_welcomeMessage;

        /// Command dictionary
        std::vector<std::string> m_commands;

        /// Remember previously entered commands
        std::deque<std::string> m_history;

        std::deque<std::string>::reverse_iterator m_prev;

        struct termios m_oldt, m_newt;

        void setupTerminal()
        {
            // See http://stackoverflow.com/questions/1798511/how-to-avoid-press-enter-with-any-getchar
            // tcgetattr gets the parameters of the current terminal
            // STDIN_FILENO will tell tcgetattr that it should write the settings
            // of stdin to m_oldt

            tcgetattr( STDIN_FILENO, &m_oldt);
            // now the settings will be copied
            m_newt = m_oldt;

            // ICANON normally takes care that one line at a time will be processed
            // that means it will return if it sees a "\n" or an EOF or an EOL
            m_newt.c_lflag &= ~(ICANON);

            // also disable standard output -- Ben
            m_newt.c_lflag &= ~ECHO;
            m_newt.c_lflag |= ECHONL;
            m_newt.c_lflag &= ICRNL;

            // Those new settings will be set to STDIN
            // TCSANOW tells tcsetattr to change attributes immediately.
            tcsetattr( STDIN_FILENO, TCSANOW, &m_newt);
        }

        void handleCharInsert(int &cursorPos, char const ch, std::string &toReturn)
        {
            // If anything was hacky as fuck, this is surely it.
            std::string str;
            str.push_back(ch);
            std::cout<<str;
            if(!toReturn.empty()) {
                std::string copy(std::begin(toReturn), std::begin(toReturn) + cursorPos);
                copy.push_back(ch);
                std::string remaining(std::begin(toReturn) + cursorPos, std::end(toReturn));
                std::cout<<remaining<<" \b";
                for(int i = 0; i < remaining.length(); ++i) {
                    std::cout<<"\b";
                }
                copy.append(remaining);
                copy.swap(toReturn);
            } else {
                toReturn.push_back(ch);
            }
            ++cursorPos;
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
                auto before = cursorPos;
                std::string copy(std::begin(toReturn), std::begin(toReturn) + cursorPos);
                std::string remaining(std::begin(toReturn) + cursorPos + 1, std::end(toReturn));
                copy.append(remaining);
                copy.swap(toReturn);

                // Remove character from end and 'slide back' string
                std::cout<<remaining<<" \b";

                // Move cursor back to correct position
                for(int i = 0; i < remaining.length(); ++i) {
                    std::cout<<"\b";
                }
            }
        }

        void handleCtrlA(int &cursorPos)
        {
            auto const pos = cursorPos;
            for(int i = 0; i < pos; ++i) {
                 --cursorPos;
                std::cout<<"\b";
            }
        }

        void handleCtrlE(int &cursorPos, std::string const & in)
        {
            handleCtrlA(cursorPos);

            for(int i = 0; i < in.length(); ++i) {
                 ++cursorPos;
            }
            std::cout<<in;
        }

        void handleCtrlK(int &cursorPos, std::string & in)
        {
            std::string copy(std::begin(in), std::begin(in) + cursorPos);
            
            // Remove from cursor to end of line
            auto const dist = std::distance(std::begin(in) + cursorPos, std::end(in));
            for(int i = 0; i < dist; ++i) {
                std::cout<<" ";
            }

            // Put cursor back
            for(int i = 0; i < dist; ++i) {
                std::cout<<"\b";
            }
            in.swap(copy);
        }

        void handleLeftArrow(int &cursorPos)
        {
            // check that the cursor position is > 0 to prevent accidental
            // deletion of prompt!
            if(cursorPos > 0) {
                --cursorPos;
                std::cout<<"\b";
            }
        }

        void handleRightArrow(int &cursorPos, std::string &toReturn)
        {
            // check that the cursor position is > 0 to prevent accidental
            // deletion of prompt!
            if(cursorPos < toReturn.length()) {
                std::string str;
                str.push_back(toReturn[cursorPos]);
                std::cout<<str;
                ++cursorPos;
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

        /// When up or down cursor is pressed
        void handleHistory(int &cursorPos,
                           std::string &toReturn,
                           bool const up = true /* cursor was up-pressed */)
        {

            if(!m_history.empty()) {
                if(up) {
                    if (m_prev != m_history.rend()) 
                    {
                        if(!toReturn.empty()) {
                            auto lenBefore = toReturn.length();
                            for(int i = 0;i < lenBefore; ++i) {
                                removeLastChar();
                            }
                        }
                        toReturn = *m_prev;
                        cursorPos = toReturn.length();
                        std::cout<<toReturn;

                        // Boundary condition
                        if(m_prev < m_history.rend()-1) {
                            ++m_prev;
                        }

                    }
                } else {
                    if(m_prev != m_history.rbegin()) 
                    {
                        --m_prev;

                        if(!toReturn.empty()) {
                            auto lenBefore = toReturn.length();
                            for(int i = 0;i < lenBefore; ++i) {
                                removeLastChar();
                            }
                        }
                        toReturn = *m_prev;
                        cursorPos = toReturn.length();
                        std::cout<<toReturn;

                        // Boundary condition
                        if(m_prev == m_history.rbegin()) {
                            ++m_prev;
                        }
                        
                    }
                }
            }
        }

        /// Arrows designated by 27, 91 and then one of {65, 66, 67, 68}
        int checkArrowCode(int const code)
        {
            if(code == 27) {
                auto following = static_cast<int>(::getchar());
                if(following == 91) {
                    auto final = static_cast<int>(::getchar());
                    return final;
                }
            }
            return -1;
        }

        /// gets a user-inputted string until return is entered
        /// will use getChar to process characters one by one so that individual
        /// key handlers can be created
        std::string getInputString()
        {
            std::string toReturn("");

            int cursorPos(0);
            while(1) {
                char c = ::getchar();
                auto intCode = static_cast<int>(c);
                auto onEnter = false;
                switch(intCode) {
                    case SP_ENTER:
                      std::cout<<std::endl;
                      onEnter = true;
                      break;  
                    case SP_CTRL_C:
                      exit(0);
                    case SP_CTRL_A:
                      handleCtrlA(cursorPos);
                      break;
                    case SP_CTRL_E:
                      handleCtrlE(cursorPos, toReturn);
                      break;
                    case SP_CTRL_K:
                      handleCtrlK(cursorPos, toReturn);
                      break;
                    case SP_BACKSPACE:
                    case SP_DELETE:
                      handleBackspace(cursorPos, toReturn);
                      break;
                    case SP_TAB:
                      handleTabKey(cursorPos, toReturn);
                      break;
                    default:
                      {
                          auto const ac = checkArrowCode(intCode);
                          if(ac == SP_UP_ARROW) {
                              handleHistory(cursorPos, toReturn);
                          } else if(ac == SP_DOWN_ARROW) {
                              handleHistory(cursorPos, toReturn, false);
                          } else if(ac == SP_LEFT_ARROW) {
                              handleLeftArrow(cursorPos);
                          } else if(ac == SP_RIGHT_ARROW) {
                              handleRightArrow(cursorPos, toReturn);
                          }  else if(ac > -1) {
                              continue; // (i.e., ignore)
                          } else {
                              // Codes <= 28 seem to correspond to
                              // non-printable characters and are attained
                              // by pressing ctrl-key
                              if(intCode > 28) {
                                  handleCharInsert(cursorPos, c, toReturn);
                                  auto remCursorPos = toReturn.length() - cursorPos;
                                  handleCtrlE(cursorPos, toReturn);
                                  for(int i = 0; i < toReturn.length(); ++i) {
                                      std::cout<<"\b";
                                  }
                                  m_printer(toReturn);
                                  for(int i = 0;i<remCursorPos; ++i) {
                                      handleLeftArrow(cursorPos);
                                  }
                              }
                          }
                      }
                }
                if(onEnter) {
                    break;
                }
            }
            return toReturn;
        }

        /// indefinitely loops over user input
        void loop()
        {
            while (1) {
                std::cout<<m_prompt;
                auto const commandStr = getInputString();
                if(!commandStr.empty()) {
                    if(m_comCallback) {
                        m_comCallback(commandStr);
                    }
                    m_history.push_back(commandStr);
                    m_prev = m_history.rbegin();
                }
            }
        }
    };
}