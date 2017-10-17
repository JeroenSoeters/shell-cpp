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

   struct part
      : plus< sor < alnum, one< '_' >, one< '-' >, one< '/' >, one< '.' > > >
   {
   };

   struct exit
      : keyword< 'e', 'x', 'i', 't' >
   {
   };

   struct cd
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

   struct exit_command
       : seq<
            exit,
            opt< whitespace >
         >
   {
   };

   struct change_directory_command
      : seq<
           cd,
           whitespace,
           directory,
           opt< whitespace >
        >
   {
   };

   struct shell_action
      : seq<
           opt< whitespace >,
           sor< 
              exit_command, 
              change_directory_command, 
              commandline 
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
