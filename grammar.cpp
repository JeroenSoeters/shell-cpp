#include "grammar.h"
#include "shell.h"

#include <iostream>

// Actions that are applied when parsing rules succeed.
namespace grammar
{
   using namespace shell;

   template< typename Rule >
      struct action
      : nothing< Rule >
      {
      };

   template<>
      struct action< nop >
      {
         static void apply0( shell::shell_state& state ) 
         {
            state.action = new NopAction();
         }
      };

   template<>
      struct action< exit >
      {
         static void apply0( shell::shell_state& state )
         {
            state.action = new ExitAction();
         };
      };

   template<>
      struct action< directory >
      {
         template< typename Input >
            static void apply( const Input& in, shell::shell_state& state )
            {
               state.action = new ChangeDirectoryAction( in.string() );
            }
      };

   template<>
      struct action< arg >
      {
         template< typename Input >
            static void apply( const Input& in, shell::shell_state& state )
            {
               RunCommandsAction * cmdl;

               if ( state.action == 0 ) {
                  state.action = new RunCommandsAction();
               }

               cmdl = static_cast< RunCommandsAction* >( state.action );
               
               if ( cmdl->numberOfCommands == 0 ) {
                  shell::command *cmd = new shell::command;
                  cmdl->commands.push_back( cmd );
                  cmdl->numberOfCommands++;
               }

               cmdl->commands.back()->args.push_back( in.string() );
            };
      };

   template<>
      struct action< background >
      {
         static void apply0( shell::shell_state& state )
         {
            RunCommandsAction * cmdl;

            cmdl = static_cast< RunCommandsAction* >( state.action );
            cmdl->runInBackground = true;
         };
      };

   template<>
      struct action< input_file >
      {
         template< typename Input >
            static void apply( const Input& in, shell::shell_state& state )
            {
               RunCommandsAction * cmdl;

               cmdl = static_cast< RunCommandsAction* >( state.action );
               cmdl->commands.back()->input_file = in.string();
            };
      };

   template<>
      struct action< output_file >
      {
         template< typename Input >
            static void apply( const Input& in, shell::shell_state& state )
            {
               RunCommandsAction * cmdl;

               cmdl = static_cast< RunCommandsAction* >( state.action );
               cmdl->commands.back()->output_file = in.string();
            };
      };

   template<>
      struct action< pipe >
      {
         static void apply0( shell::shell_state& state )
         {
            RunCommandsAction * cmdl;

            cmdl = static_cast< RunCommandsAction* >( state.action );
            cmdl->commands.push_back( new shell::command );
            cmdl->numberOfCommands++;
         };
      };
}
