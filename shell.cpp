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

#include <array>
#include <list>
#include <experimental/optional>
#include <vector>

#include <tao/pegtl.hpp>


namespace shell
{

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

      bool runInBackground;

      command *pop_first_command() {
         command *first = commands.front();
         commands.pop_front();

         return first;
      }
   };
}

namespace grammar
{
   // grammar
   using namespace tao::pegtl;

   struct whitespace
      : plus< blank >
   {
   };

   struct part
      : plus< sor < alnum, one< '_' >, one< '-' >, one< '/' >, one< '.' > > >
   {
   };

   struct arg
      : part
   {
   };

   struct input_file
      : part
   {
   };

   struct output_file
      : part
   {
   };

   struct pipe
      : one< '|' >
   {
   };

   struct background
      : one< '&' >
   {
   };

   struct command
      : seq<
        arg,
        star< seq< whitespace, arg > >
        >
   {
   };

   struct redir_stdin
      : seq<
        one< '<' >,
        opt< whitespace >,
        input_file
        >
   {
   };

   struct redir_stdout
      : seq<
        one< '>' >,
        opt< whitespace >,
        output_file
        >
   {
   };

   struct commandline
      : seq<
        opt< whitespace >,
        command,
        opt< whitespace >,
        opt< redir_stdin >,
        opt< whitespace >,
        star< seq< pipe, opt< whitespace >, command, opt< whitespace > > >,
        opt< redir_stdout >,
        opt< whitespace >,
        opt< background >,
        opt< whitespace >
        >
   {
   };

   struct grammar
      : must< commandline, eolf >
   {
   };

   template< typename Rule >
      struct action
      : nothing< Rule >
      {
      };

   template<>
      struct action< arg >
      {
         template< typename Input >
            static void apply( const Input& in, shell::commandline& cmdl )
            {
               if ( cmdl.numberOfCommands == 0 ) {
                  shell::command *cmd = new shell::command;
                  cmdl.commands.push_back( cmd );
                  cmdl.numberOfCommands++;
               }

               cmdl.commands.back()->args.push_back( in.string() );
            };
      };

   template<>
      struct action< background >
      {
         static void apply0( shell::commandline& cmdl )
         {
            cmdl.runInBackground = true;
         };
      };

   template<>
      struct action< input_file >
      {
         template< typename Input >
            static void apply( const Input& in, shell::commandline& cmdl )
            {
               cmdl.commands.back()->input_file = in.string();
            };
      };

   template<>
      struct action< output_file >
      {
         template< typename Input >
            static void apply( const Input& in, shell::commandline& cmdl )
            {
               cmdl.commands.back()->output_file = in.string();
            };
      };

   template<>
      struct action< pipe >
      {
         static void apply0( shell::commandline& cmdl )
         {
            cmdl.commands.push_back( new shell::command );
            cmdl.numberOfCommands++;
         };
      };
}

namespace shell
{
   char** convert_to_c_args( std::vector< std::string > args ) {
      char** c_args = new char*[args.size()+1];
      for ( int i = 0; i < args.size(); ++i )
         c_args[i] = strdup( args[i].c_str() );
      c_args[args.size()] = NULL;

      return c_args;
   }

   void free_c_args( char** c_args, int number_of_c_args ) {
      for (int i = 0; i < number_of_c_args; ++i)
         free( c_args[i] );
      delete[] c_args;
   }

   void overlayProcess( std::vector< std::string > args ) {
      char** c_args = convert_to_c_args( args );
      execvp( c_args[0], c_args );

      // there must be an error if we get to here
      free_c_args( c_args, args.size() );

      std::exit( EXIT_FAILURE );
   }

   void read_from_file( std::string input_file ) {
      int fd = open( input_file.c_str(), O_RDONLY, S_IRUSR );
      close( STDIN_FILENO );
      dup( fd );
   }

   void write_to_file( std::string output_file ) { 
      int fd = open( output_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
      close( STDOUT_FILENO );
      dup( fd );
   }

   void read_from_pipe( std::array< int, 2 > pipe ) {
      dup2( pipe[0], 0 );
      close( pipe[1] );
   }

   void write_to_pipe( std::array< int, 2 > pipe ) {
      close( pipe[0] );
      dup2( pipe[1], 1 );
   }

   void close_pipe( std::array< int, 2 > pipe ) {
      close( pipe[0] );
      close( pipe[1] );
   }

   pid_t execute_chained( command* cmd, bool has_prev_pipe, std::array< int, 2 > prev_pipe, bool has_next_pipe, std::array< int, 2 > next_pipe ) {
      pid_t pid;

      if ( (pid = fork()) < 0 ) {
         std::exit( EXIT_FAILURE );                         // Fork failed.
      }
      else if ( pid == 0 ) {                                // In child process.
         if ( has_prev_pipe ) {                             // If there is a previous pipe, read from it.
            read_from_pipe( prev_pipe );
         } 
         else if ( cmd->has_file_input() ) {                // First process in the chain, check if input should be read from file.
            read_from_file( cmd->input_file );
         }
         
         if ( has_next_pipe ) {
            write_to_pipe( next_pipe );                     // If there is a next pipe, write to it.
         }
         else if ( cmd->has_file_output() ) {               // Last process in the chain, check if output should be written to file.
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

   int wait_for_process_chain( std::list< pid_t > pids ) {
      int status;

      while ( pids.size() > 1 ) {
         waitpid( pids.front(), NULL, WUNTRACED );
         pids.pop_front();
      }
      waitpid( pids.front(), &status, WUNTRACED );

      return status;
   }

   int execute_commandline( commandline& cmdl ) {
      std::list < pid_t > pids;
      std::array< int, 2 > prev_pipe, next_pipe;
      bool has_prev, has_next;
      int i;
      command *cmd;

      for ( i = 0; i < cmdl.numberOfCommands ; i++ ) {      
         cmd = cmdl.pop_first_command();                    // Pop the first command
         has_next = i != cmdl.numberOfCommands - 1;         

         if ( pipe( next_pipe.data() ) < 0 ) {
            std::exit( EXIT_FAILURE );                      // Pipe failed
         }

         pids.push_front( 
            execute_chained( cmd, has_prev, prev_pipe, has_next, next_pipe ) 
         );
    
         prev_pipe = next_pipe;
         has_prev = true;
      }

      return wait_for_process_chain( pids );
   }

   void display_prompt() {
      char buffer[512];
      char* dir = getcwd(buffer, sizeof(buffer));
      if (dir)
         std::cout << "\e[32m" << dir << "\e[39m"; // the strings starting with '\e' are escape codes, that the terminal application interpets in this case as "set color to green"/"set color to default"
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

   void parse_command( std::string input, commandline& cmdl ) {
      grammar::string_input<> in( input, "std::string" );
      tao::pegtl::parse< grammar::grammar, grammar::action >( in, cmdl );
   }
}

int run_shell( bool show_prompt ) {
   using namespace shell;

   commandline cmdl;

   std::string input = request_commandLine(show_prompt);
   parse_command( input, cmdl );

   int rc = execute_commandline( cmdl );
   if ( rc != 0 )
      std::cout << strerror( rc ) << std::endl;

   return 0;
}
