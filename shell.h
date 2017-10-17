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

   extern char** convert_to_c_args( std::vector< std::string > args );

   extern void free_c_args( char** c_args, int number_of_c_args );

   extern void overlayProcess( std::vector< std::string > args );

   extern void read_from_file( std::string input_file );

   extern void write_to_file( std::string output_file );

   extern void read_from_pipe( std::array< int, 2 > pipe );

   extern void write_to_pipe( std::array< int, 2 > pipe );

   extern void close_pipe( std::array< int, 2 > pipe );

   extern pid_t execute_chained( command* cmd, bool has_prev_pipe, std::array< int, 2 > prev_pipe, bool has_next_pipe, std::array< int, 2 > next_pipe );

   extern int wait_for_process_chain( std::list< pid_t > pids );

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

   struct shell_action
   {
      virtual int execute() = 0;
   };

   struct exit_action : shell_action
   {
      int execute() {
         exit( EXIT_SUCCESS );
      }
   };

   struct change_directory_action : shell_action
   {
      std::string new_directory;

      int execute() {
         int rc = chdir( new_directory.c_str() );
         if ( rc != 0 ) {
            switch ( errno ) {
               case ENOENT:
                  std::cout << "No such file or directory";
                  break;
               default:
                  std::cout << "Unknown error";
            }
         }
         return rc;
      }
   };

   struct run_commands_action : shell_action
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

      int execute() {
         std::list < pid_t > pids;
         std::array< int, 2 > prev_pipe, next_pipe;
         bool has_prev, has_next;
         int i;
         command *cmd;

         for ( i = 0; i < numberOfCommands ; i++ ) {      
            cmd = pop_first_command();                    // Pop the first command.
            has_next = i != numberOfCommands - 1;         

            if ( pipe( next_pipe.data() ) < 0 ) {
               std::exit( EXIT_FAILURE );                      // Pipe failed.
            }

            pids.push_front( 
                  execute_chained( cmd, has_prev, prev_pipe, has_next, next_pipe ) 
                  );

            prev_pipe = next_pipe;                             // The output pipe for the current process will be the input pipe for the next process.
            has_prev = true;
         }

         return wait_for_process_chain( pids );
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

   struct shell_state
   {
      shell_action *action = 0;
   };

   void parse_command( std::string input, shell_state& state );
}
#endif
