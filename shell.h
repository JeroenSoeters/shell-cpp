#ifndef SHELL_H
#define SHELL_H

#include <array>
#include <list>
#include <vector>
#include <unistd.h>

#include <iostream>

namespace shell 
{
   struct command;

   class ShellAction
   {
   public:
      virtual int execute() = 0;
   };

   class NopAction: public ShellAction
   {
   public:
      NopAction() noexcept;
      int execute() noexcept;
   };

   class ExitAction: public ShellAction
   {
   public:
      ExitAction() noexcept;
      int execute() noexcept;
   };

   class ChangeDirectoryAction: public ShellAction
   {
   public:
      std::string new_directory;
      ChangeDirectoryAction( std::string directory ) noexcept;
      int execute() noexcept;
   };

   class RunCommandsAction: public ShellAction
   {
   private:
      char** convert_to_c_args( std::vector< std::string > args ) noexcept;
      void free_c_args( char** c_args, int number_of_c_args ) noexcept;
      void overlayProcess( std::vector< std::string > args ) noexcept;
      void read_from_file( std::string input_file ) noexcept;
      void write_to_file( std::string output_file ) noexcept; 
      void read_from_pipe( std::array< int, 2 > pipe ) noexcept;
      void write_to_pipe( std::array< int, 2 > pipe ) noexcept;
      void close_pipe( std::array< int, 2 > pipe ) noexcept;
      pid_t execute_chained( command* cmd, bool has_prev_pipe, std::array< int, 2 > prev_pipe, bool has_next_pipe, std::array< int, 2 > next_pipe ) noexcept;
      int wait_for_process_chain( std::list< pid_t > pids ) noexcept;
   public:
      int numberOfCommands = 0;
      std::list< command* > commands;
      bool runInBackground = false;
      RunCommandsAction() noexcept;
      int execute() noexcept;
      command *peek_first_command() noexcept;
      command *pop_first_command() noexcept;
   };

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

   struct shell_state
   {
      ShellAction * action = 0;
   };

   void parse_command( std::string input, shell_state& state );
}
#endif
