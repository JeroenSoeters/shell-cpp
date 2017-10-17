#include "grammar.h"
#include "shell.h"

#include <iostream>

// Actions that are applied when parsing rules succeed.
namespace grammar
{
   template< typename Rule >
      struct action
      : nothing< Rule >
      {
      };

   template<>
      struct action< exit >
      {
         static void apply0( shell::shell_state& state )
         {
            state.action = new shell::exit_action;
         };
      };

   template<>
      struct action< cd >
      {
         static void apply0( shell::shell_state& state )
         {
            state.action = new shell::change_directory_action;
         };
      };

   template<>
      struct action< directory >
      {
         template< typename Input >
            static void apply( const Input& in, shell::shell_state& state )
            {
               shell::change_directory_action * chdir;
               
               chdir = static_cast< shell::change_directory_action* >( state.action );
               chdir->new_directory = in.string();
            }
      };

   template<>
      struct action< arg >
      {
         template< typename Input >
            static void apply( const Input& in, shell::shell_state& state )
            {
               shell::run_commands_action * cmdl;

               if ( state.action == 0 ) {
                  state.action = new shell::run_commands_action;
               }

               cmdl = static_cast< shell::run_commands_action* >( state.action );
               
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
            shell::run_commands_action * cmdl;

            cmdl = static_cast< shell::run_commands_action* >( state.action );
            cmdl->runInBackground = true;
         };
      };

   template<>
      struct action< input_file >
      {
         template< typename Input >
            static void apply( const Input& in, shell::shell_state& state )
            {
               shell::run_commands_action * cmdl;

               cmdl = static_cast< shell::run_commands_action* >( state.action );
               cmdl->commands.back()->input_file = in.string();
            };
      };

   template<>
      struct action< output_file >
      {
         template< typename Input >
            static void apply( const Input& in, shell::shell_state& state )
            {
               shell::run_commands_action * cmdl;

               cmdl = static_cast< shell::run_commands_action* >( state.action );
               cmdl->commands.back()->output_file = in.string();
            };
      };

   template<>
      struct action< pipe >
      {
         static void apply0( shell::shell_state& state )
         {
            shell::run_commands_action * cmdl;

            cmdl = static_cast< shell::run_commands_action* >( state.action );
            cmdl->commands.push_back( new shell::command );
            cmdl->numberOfCommands++;
         };
      };
}


