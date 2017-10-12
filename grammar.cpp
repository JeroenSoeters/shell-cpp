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


