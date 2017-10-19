#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <tao/pegtl.hpp>

// Parsing rules.
namespace grammar {

   using namespace tao::pegtl;

   struct whitespace
      : plus< blank >
   {
   };

   struct optional_whitespace
      : opt< whitespace >
   {
   };

   struct part
      : plus< sor < alnum, one< '_' >, one< '-' >, one< '/' >, one< '.' > > >
   {
   };

   struct nop
      : optional_whitespace
   {
   };

   struct exit_keyword
      : keyword< 'e', 'x', 'i', 't' >
   {
   };

   struct cd_keyword
      : keyword< 'c', 'd' >
   {
   };

   struct directory
      : part
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
           optional_whitespace,
           input_file
        >
   {
   };

   struct redir_stdout
      : seq<
           one< '>' >,
           optional_whitespace,
           output_file
        >
   {
   };

   struct run_commands
      : seq<
           optional_whitespace,
           command,
           optional_whitespace,
           opt< redir_stdin >,
           optional_whitespace,
           star< seq< pipe, optional_whitespace, command, optional_whitespace > >,
           opt< redir_stdout >,
           optional_whitespace,
           opt< background >,
           optional_whitespace
        >
   {
   };

   struct exit
       : seq<
            optional_whitespace,
            exit_keyword,
            optional_whitespace
         >
   {
   };

   struct change_directory
      : seq<
           optional_whitespace,
           cd_keyword,
           whitespace,
           directory,
           optional_whitespace
        >
   {
   };

   struct shell_action
      : seq<
           sor< 
              exit, 
              change_directory, 
              run_commands,
              nop
           >
        >
   {
   };

   struct grammar
      : must< shell_action, eolf >
   {
   };
}
#endif
