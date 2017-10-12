#ifndef SHELL_H
#define SHELL_H

#include <list>

namespace shell {

   struct command
   {
      std::vector< std::string > args;
      std::string input_file, output_file;

      bool has_file_input() {
         return input_file != "";
      }

      bool has_file_output() {
         return output_file != "";
      }
   };

   struct commandline
   {
      int numberOfCommands = 0;
      std::list< command* > commands;

      bool runInBackground = false;

      command *peek_first_command() {
         return commands.front();
      }

      command *pop_first_command() {
         command *first = commands.front();
         commands.pop_front();

         return first;
      }
   };

   void parse_command( std::string input, commandline& cmdl );
}
#endif
