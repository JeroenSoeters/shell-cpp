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

   struct exit
       : keyword< 'e', 'x', 'i', 't' >
   {
   };

   struct change_directory
      : seq<
           keyword< 'c', 'd' >,
           whitespace,
           arg
        >
   {
   };

   struct shell_action
      : sor< exit, change_directory, commandline >
   {
   };

   struct grammar
      : must< shell_action, eolf >
   {
   };
}
#endif
