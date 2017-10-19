#include <gtest/gtest.h>
#include <stdlib.h>
#include <fcntl.h>

#include <list>

#include "grammar.h"
#include "shell.h"

using namespace std;
using namespace shell;

// shell to run tests on
#define SHELL "../build/shell -t"

namespace {
   void execute( std::string command, std::string expectedOutput );
   void execute( std::string command, std::string expectedOutput, std::string expectedErrors );
   void execute( std::string command, std::string expectedOutput, std::string expectedOutputFile, std::string expectedOutputFileContent );
   bool try_parse_nop_action( std::string input, NopAction **nop );
   bool try_parse_exit_action( std::string input, ExitAction **exit);
   bool try_parse_change_directory_action( std::string input, ChangeDirectoryAction **change_directory );
   bool try_parse_run_commands_action( std::string input, RunCommandsAction **run_commands );
   bool try_parse_single_command( std::string input, command **cmd );

   TEST( Shell, ParseNop ) {
      NopAction *nop = nullptr;

      EXPECT_TRUE( try_parse_nop_action( " ", &nop ) );
      EXPECT_TRUE( try_parse_nop_action( "    ", &nop ) );
   }
   
   TEST( Shell, ParseExit ) {
      ExitAction *exit = nullptr;

      EXPECT_TRUE( try_parse_exit_action( "exit", &exit ) );
      EXPECT_TRUE( try_parse_exit_action( "exit ", &exit ) );
   }

   TEST( Shell, ParseChangeDirectory ) {
      ChangeDirectoryAction * chdir = NULL;
      std::string expected_dir;

      expected_dir = "tmp";

      EXPECT_TRUE( try_parse_change_directory_action( "cd tmp", &chdir ) );
      EXPECT_EQ( expected_dir, chdir->new_directory );

      expected_dir = "tmp";

      EXPECT_TRUE( try_parse_change_directory_action( "cd tmp ", &chdir ) );
      EXPECT_EQ( expected_dir, chdir->new_directory );

      expected_dir = "tmp";

      EXPECT_TRUE( try_parse_change_directory_action( "cd tmp ", &chdir ) );
      EXPECT_EQ( expected_dir, chdir->new_directory );

      expected_dir = "..";

      EXPECT_TRUE( try_parse_change_directory_action( "cd ..", &chdir ) );
      EXPECT_EQ( expected_dir, chdir->new_directory );
   }

   TEST( Shell, ParseSingleCommandWithoutArguments ) {
      command *cmd = nullptr;
      std::vector<std::string> expected_args;

      expected_args = { "foo" };

      EXPECT_TRUE( try_parse_single_command( "foo", &cmd ) );
      EXPECT_EQ( expected_args, cmd->args );

      EXPECT_TRUE( try_parse_single_command( " foo", &cmd ) );
      EXPECT_EQ( expected_args, cmd->args );

      EXPECT_TRUE( try_parse_single_command( " foo ", &cmd ) );
      EXPECT_EQ( expected_args, cmd->args );

      EXPECT_TRUE( try_parse_single_command( "  foo  ", &cmd ) );
      EXPECT_EQ( expected_args, cmd->args );

      EXPECT_TRUE( try_parse_single_command( "   foo   ", &cmd ) );
      EXPECT_EQ( expected_args, cmd->args );
   }

   TEST( Shell, ParseSingleCommandWithArguments ) {
      command *cmd = nullptr;
      std::vector<std::string> expected_args;

      expected_args = { "cmd", "1", "-n", "u" };

      EXPECT_TRUE( try_parse_single_command( "cmd 1 -n u", &cmd ) );
      EXPECT_EQ( expected_args, cmd->args );
   }

   TEST( Shell, ParseSingleCommandNoArgumentsWithRedirectStdin ) {
      command *cmd = nullptr;
      std::vector<std::string> expected_args;
      std::string expected_input_file;

      expected_args = { "cmd" };
      expected_input_file = "inputfile";

      try_parse_single_command( "cmd < inputfile", &cmd );

      EXPECT_EQ( expected_args, cmd->args );
      EXPECT_EQ( expected_input_file, cmd->input_file );
      EXPECT_EQ( "", cmd->output_file );
   }

   TEST( Shell, ParseSingleCommandWithRedirectStdin ) {
      command *cmd = nullptr;
      std::vector<std::string> expected_args;
      std::string expected_input_file;

      expected_args = { "cmd", "arg1" };
      expected_input_file = "inputfile";

      try_parse_single_command( "cmd arg1 < inputfile", &cmd );

      EXPECT_EQ( expected_args, cmd->args );
      EXPECT_EQ( expected_input_file, cmd->input_file );
      EXPECT_EQ( "", cmd->output_file );
   }

   TEST( Shell, ParseSingleCommandWithRedirectStdout ) {
      command *cmd = nullptr;
      std::vector<std::string> expected_args;
      std::string expected_output_file;

      expected_args = { "cmd", "arg1" };
      expected_output_file = "outputfile";

      try_parse_single_command( "cmd arg1 > outputfile", &cmd );

      EXPECT_EQ( expected_args, cmd->args );
      EXPECT_EQ( expected_output_file, cmd->output_file );
      EXPECT_EQ( "", cmd->input_file );
   }

   TEST( Shell, ParseSingleCommandWithRedirectStdinAndRedirectStdout ) {
      command *cmd = nullptr;
      std::vector<std::string> expected_args;
      std::string expected_input_file, expected_output_file;

      expected_args = { "cmd", "arg1" };
      expected_input_file = "inputfile";
      expected_output_file = "outputfile";

      try_parse_single_command( "cmd arg1 < inputfile > outputfile", &cmd );

      EXPECT_EQ( expected_args, cmd->args );
      EXPECT_EQ( expected_output_file, cmd->output_file );
      EXPECT_EQ( expected_input_file, cmd->input_file );
   }

   TEST( Shell, ParseRunCommands ) {
      RunCommandsAction * run_commands;
      int expected_number_of_commands;
      std::vector< std::vector< std::string > > expected_args;

      expected_number_of_commands = 3;
      expected_args = { { "ls" "-la" }, { "sort" }, { "uniq -i" } };

      try_parse_run_commands_action( "ls -la | sort | uniq -l", &run_commands );

      EXPECT_EQ( expected_number_of_commands, run_commands->numberOfCommands );
      EXPECT_FALSE( run_commands->runInBackground );
   }

   TEST( Shell, ParseSingleCommandThatRunsInBackground ) {
      RunCommandsAction * run_commands;
      std::vector<std::string> expected_args;

      expected_args = { "foo" };

      try_parse_run_commands_action( "foo &", &run_commands );

      EXPECT_EQ( expected_args, run_commands->commands.front()->args );
      EXPECT_TRUE( run_commands->runInBackground );
   }

   TEST( Shell, ParsePipelineThatRunsInBackground ) {
      RunCommandsAction * run_commands;

      try_parse_run_commands_action( "cat < inputfile | sort | uniq &", &run_commands );

      EXPECT_TRUE( run_commands->runInBackground );
   }

   TEST( Shell, ParsePipelineWithRedirectStdinAndRedirectStdout ) {
      RunCommandsAction * run_commands;
      int expected_number_of_commands;
      std::vector< std::vector< std::string > > expected_args;
      std::string expected_input_file, expected_output_file;

      expected_number_of_commands = 3;
      expected_args = { { "ls" "-la" }, { "sort" }, { "uniq -i" } };
      expected_input_file = "inputfile";
      expected_output_file = "outputfile";

      try_parse_run_commands_action( "ls -la < inputfile | sort | uniq -l > outputfile", &run_commands );

      EXPECT_EQ( expected_number_of_commands, run_commands->numberOfCommands );
      EXPECT_EQ( expected_input_file, run_commands->commands.front()->input_file );
      EXPECT_EQ( expected_output_file, run_commands->commands.back()->output_file );
   }

   TEST( Shell, ReadFromFile ) {
      execute("cat < 1", "line 1\nline 2\nline 3\nline 4");
   }

   TEST( Shell, ReadFromAndWriteToFile ) {
      execute("cat < 1 > ../foobar", "", "../foobar", "line 1\nline 2\nline 3\nline 4");
   }

   TEST( Shell, ReadFromAndWriteToFileChained ) {
      execute("cat < 1 | head -n 3 > ../foobar", "", "../foobar", "line 1\nline 2\nline 3\n");
      execute("cat < 1 | head -n 3 | tail -n 1 > ../foobar", "", "../foobar", "line 3\n");
   }

   TEST( Shell, WriteToFile ) {
      execute("ls -1 > ../foobar", "", "../foobar", "1\n2\n3\n4\n");
   }

   TEST( Shell, Execute ) {
      execute("uname", "Darwin\n");
      execute("ls", "1\n2\n3\n4\n");
      execute("ls -1", "1\n2\n3\n4\n");
   }

   TEST( Shell, ExecuteChained ) {
      execute("ls -1 | head -n 2", "1\n2\n");
      execute("ls -1 | head -n 2 | tail -n 1", "2\n");
   }

   TEST( Shell, ExecuteChainedWithErrors ) {
      execute( "program-that-doesnt-exist | program-that-doesnt-exist | ls", "1\n2\n3\n4\n", "command not found\ncommand not found\n" );
   }

   TEST( Shell, ExecuteInBackground ) {
      execute( "ls &", "" );
   }

   TEST( Shell, ChangeDirectory ) {
      system( "mkdir ../test-dir/nested" );
      execute( "cd nested", "", "" );
      system( "rm -rf ../test-dir/nested" );

      execute( "cd this-directory-doesnt-exist", "", "No such file or directory" );
   }

   TEST( Shell, ParseError ) {
      execute( "!", "", "command not found\n");
   }


   //////////////// HELPERS

   std::string filecontents(const std::string& str) {
      std::string retval;
      int fd = open(str.c_str(), O_RDONLY);
      struct stat st;
      if (fd >= 0 && fstat(fd, &st) == 0) {
         long long size = st.st_size;
         retval.resize(size);
         char *current = (char*)retval.c_str();
         ssize_t left = size;
         while (left > 0) {
            ssize_t bytes = read(fd, current, left);
            if (bytes == 0 || (bytes < 0 && errno != EINTR))
               break;
            if (bytes > 0) {
               current += bytes;
               left -= bytes;
            }
         }
      }
      if (fd >= 0)
         close(fd);
      return retval;
   }

   void filewrite(const std::string& str, std::string content) {
      int fd = open(str.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
      if (fd < 0)
         return;
      while (content.size() > 0) {
         int written = write(fd, content.c_str(), content.size());
         if (written == -1 && errno != EINTR) {
            std::cout << "error writing file '" << str << "': error " << errno << std::endl;
            break;
         }
         content = content.substr(written);
      }
      close(fd);
   }

   void execute( std::string command, std::string expectedOutput ) {
      filewrite( "input", command );
      system( "cd ../test-dir; " SHELL " < ../build/input > ../build/output 2> /dev/null" );
      std::string got = filecontents( "output" );
      EXPECT_EQ( expectedOutput, got );
   }

   void execute( std::string command, std::string expectedOutput, std::string expectedErrors ) {
      filewrite( "input", command );
      system( "cd ../test-dir; " SHELL " < ../build/input > ../build/output 2> ../build/errors" );
      std::string got = filecontents( "output" );
      EXPECT_EQ( expectedOutput, got );
      std::string errors = filecontents( "errors" );
      EXPECT_EQ( expectedErrors, errors );
   }

   void execute( std::string command, std::string expectedOutput, std::string expectedOutputFile, std::string expectedOutputFileContent ) {
      std::string expectedOutputLocation = "../test-dir/" + expectedOutputFile;
      unlink(expectedOutputLocation.c_str());
      filewrite("input", command);
      int rc = system("cd ../test-dir; " SHELL " < ../build/input > ../build/output 2> /dev/null");
      EXPECT_EQ(0, rc);
      std::string got = filecontents("output");
      EXPECT_EQ(expectedOutput, got) << command;
      std::string gotOutputFileContents = filecontents(expectedOutputLocation);
      EXPECT_EQ(expectedOutputFileContent, gotOutputFileContents) << command;
      unlink(expectedOutputLocation.c_str());
   }

   bool try_parse_nop_action( std::string input, NopAction **nop ) {
      shell_state state;

      parse_command( input, state );

      if ( ( *nop = static_cast< NopAction* >( state.action ) ) ) {
         return true;
      } 
      return false;
   }

   bool try_parse_exit_action( std::string input, ExitAction **exit ) {
      shell_state state;

      parse_command( input, state );

      if ( ( *exit = static_cast< ExitAction* >( state.action ) ) ) {
         return true;
      } 
      return false;
   }

   bool try_parse_change_directory_action( std::string input, ChangeDirectoryAction **change_directory ) {
      shell_state state;

      parse_command( input, state );

      if ( ( *change_directory = static_cast< ChangeDirectoryAction* >( state.action ) ) ) {
         return true;
      } 
      return false;
   }

   bool try_parse_run_commands_action( std::string input, RunCommandsAction **run_commands ) {
      shell_state state;

      parse_command( input, state );

      if ( ( *run_commands = static_cast< RunCommandsAction* >( state.action ) ) ) {
         return true;
      } 
      return false;
   }

   bool try_parse_single_command( std::string input, command **cmd ) {
      RunCommandsAction * run_commands = nullptr;

      EXPECT_TRUE( try_parse_run_commands_action( input, &run_commands ) );
      EXPECT_EQ( 1, run_commands->commands.size() );

      *cmd = run_commands->commands.front();
      
      return true;
   }
}
