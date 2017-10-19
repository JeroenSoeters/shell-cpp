/**
 * Shell
 * Operating Systems
 */

/**
Hint: F2 (or Control-click) on a functionname to go to the definition
Hint: F1 while cursor is on a system call to lookup the manual page (if it does not work, open a shell and type "man [systemcall]")
Hint: Ctrl-space to auto complete functions and variables
*/

// function/class definitions you are going to use
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <tao/pegtl.hpp>

#include "grammar.cpp"                                      // Would be nicer to extract this into a header file.
#include "shell.h"


namespace shell
{
   NopAction::NopAction() noexcept { }
   int NopAction::execute() noexcept { return 0; }

   ExitAction::ExitAction() noexcept { }
   int ExitAction::execute() noexcept 
   {
      exit( EXIT_SUCCESS );
   }

   ChangeDirectoryAction::ChangeDirectoryAction( std::string directory ) noexcept 
   {
      new_directory = directory;
   }
   int ChangeDirectoryAction::execute() noexcept 
   {
      int rc = chdir( new_directory.c_str() );
      if ( rc != 0 ) {
         switch ( errno ) {
            case ENOENT:
               std::cerr << "No such file or directory";
               break;
            default:
               std::cerr << "Unknown error";
         }
      }
      return rc;
   }
   
   RunCommandsAction::RunCommandsAction() noexcept { }
   char** RunCommandsAction::convert_to_c_args( std::vector< std::string > args ) noexcept 
   {
      char** c_args = new char*[args.size()+1];
      for ( int i = 0; i < args.size(); ++i )
         c_args[i] = strdup( args[i].c_str() );
      c_args[args.size()] = NULL;

      return c_args;
   }
   void RunCommandsAction::free_c_args( char** c_args, int number_of_c_args ) noexcept 
   {
      for (int i = 0; i < number_of_c_args; ++i)
         free( c_args[i] );
      delete[] c_args;
   }
   void RunCommandsAction::overlayProcess( std::vector< std::string > args ) noexcept 
   {
      char** c_args = convert_to_c_args( args );
      execvp( c_args[0], c_args );
      // there must be an error if we get to here
      free_c_args( c_args, args.size() );
      switch ( errno ) {
         case ENOENT:
            std::cerr << "command not found\n";
            break;
         default:
            std::cerr << "unknown error\n";
      }
      std::exit( EXIT_FAILURE );
   }
   void RunCommandsAction::read_from_file( std::string input_file ) noexcept 
   {
      int fd = open( input_file.c_str(), O_RDONLY, S_IRUSR );
      close( STDIN_FILENO );
      dup( fd );
   }
   void RunCommandsAction::write_to_file( std::string output_file ) noexcept 
   { 
      int fd = open( output_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
      close( STDOUT_FILENO );
      dup( fd );
   }
   void RunCommandsAction::read_from_pipe( std::array< int, 2 > pipe ) noexcept 
   {
      dup2( pipe[0], 0 );
      close( pipe[1] );
   }
   void RunCommandsAction::write_to_pipe( std::array< int, 2 > pipe ) noexcept 
   {
      close( pipe[0] );
      dup2( pipe[1], 1 );
   }
   void RunCommandsAction::close_pipe( std::array< int, 2 > pipe ) noexcept 
   {
      close( pipe[0] );
      close( pipe[1] );
   }
   bool RunCommandsAction::has_file_input( command * cmd ) noexcept
   {
      return cmd->input_file != "";
   }
   bool RunCommandsAction::has_file_output( command * cmd ) noexcept 
   {
      return cmd->output_file != "";
   }
   // This would have been nicer with std::optional (C++17) which doesn't compile on MacOSX.
   pid_t RunCommandsAction::execute_chained( command* cmd, bool has_prev_pipe, std::array< int, 2 > prev_pipe, bool has_next_pipe, std::array< int, 2 > next_pipe ) noexcept 
   {
      pid_t pid;

      if ( (pid = fork()) < 0 ) {
         std::exit( EXIT_FAILURE );                         // Fork failed.
      }
      else if ( pid == 0 ) {                                // In child process.
         if ( has_prev_pipe ) {                             // If there is a previous pipe, read from it.
            read_from_pipe( prev_pipe );
         } 
         else if ( has_file_input( cmd ) ) {                // First process in the chain, check if input should be read from file.
            read_from_file( cmd->input_file );
         }

         if ( has_next_pipe ) {
            write_to_pipe( next_pipe );                     // If there is a next pipe, write to it.
         }
         else if ( has_file_output( cmd ) ) {               // Last process in the chain, check if output should be written to file.
            write_to_file( cmd->output_file );
         }

         overlayProcess( cmd->args );                       // Overlay the process image with that of the command.
      }
      else {                                                // In parent process.
         if ( has_prev_pipe ) {                             // If there is a previous pipe, close it.
            close_pipe( prev_pipe );
         };
      }

      return pid;
   }
   int RunCommandsAction::wait_for_process_chain( std::list< pid_t > pids ) noexcept 
   {
      int status;

      while ( pids.size() > 1 ) {
         waitpid( pids.front(), NULL, WUNTRACED );
         pids.pop_front();
      }
      waitpid( pids.front(), &status, WUNTRACED );

      return status;
   }
   command* RunCommandsAction::peek_first_command() noexcept
   {
      return commands.front();
   }

   command* RunCommandsAction::pop_first_command() noexcept
   {
      command *first = commands.front();
      commands.pop_front();

      return first;
   }
   int RunCommandsAction::execute() noexcept
   {
      std::list < pid_t > pids;
      std::array< int, 2 > prev_pipe, next_pipe;
      bool has_prev, has_next;
      int i;
      command *cmd;

      for ( i = 0; i < numberOfCommands ; i++ ) {      
         cmd = pop_first_command();                           // Pop the first command.
         has_next = i != numberOfCommands - 1;         

         if ( pipe( next_pipe.data() ) < 0 ) {
            std::exit( EXIT_FAILURE );                        // Pipe failed.
         }

         pids.push_front( 
               execute_chained( cmd, has_prev, prev_pipe, has_next, next_pipe ) 
               );

         prev_pipe = next_pipe;                               // The output pipe for the current process will be the input pipe for the next process.
         has_prev = true;
      }

      return runInBackground
         ? 0
         : wait_for_process_chain( pids );
   }

   void display_prompt() {
      char buffer[512];
      char* dir = getcwd(buffer, sizeof(buffer));
      if (dir)
         std::cout << "\e[32m" << dir << "\e[39m";            // the strings starting with '\e' are escape codes, that the terminal application interpets in this case as "set color to green"/"set color to default"
      std::cout << "$ ";
      std::flush(std::cout);
   }

   std::string request_commandLine( bool show_prompt ) {
      if ( show_prompt )
         display_prompt();
      std::string retval;
      getline( std::cin, retval );
 
      return retval;
   }

   void parse_command( std::string input, shell_state& state ) {
      grammar::string_input<> in( input, "std::string" );
      tao::pegtl::parse< grammar::grammar, grammar::action >( in, state );
   }

   int run_shell( bool show_prompt ) {
      using namespace shell;

      bool loop;

      loop = show_prompt;

      do {                                                     
         shell_state state;

         std::string input = request_commandLine( show_prompt ); // Request for input

         try
         {
            parse_command( input, state );                       // Parse the input into shell_state
            state.action->execute();                             // Execute the action on the state
         }
         catch ( std::exception& e )
         {
            std::cerr << "command not found" << std::endl;
         }
      } while ( loop );

      return 0;
   }
}
