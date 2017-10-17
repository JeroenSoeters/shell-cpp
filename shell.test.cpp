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
//#define SHELL "/bin/sh"

// declarations of methods you want to test (should match exactly)
//void ParseRunCommandsAction( std::string input, shell::commandline& cmdl );

namespace {
   void Execute( std::string command, std::string expectedOutput );
   void Execute( std::string command, std::string expectedOutput, std::string expectedOutputFile, std::string expectedOutputFileContent );
   nop_action*              ParseNopAction( std::string input );
   exit_action*             ParseExitAction( std::string input );
   change_directory_action* ParseChangeDirectoryAction( std::string input );
   run_commands_action*     ParseRunCommandsAction( std::string input );
   command*                 ParseSingleCommand( std::string input );

   TEST( Shell, ParseNop ) {
      nop_action *nop;

      nop = ParseNopAction( " " );
      nop = ParseNopAction( "    " );
   }
   
   TEST( Shell, ParseExit ) {
      exit_action *exit;

      exit = ParseExitAction( "exit" );      // Fails if it didn't parse correctly.
      exit = ParseExitAction( "exit " );      // Fails if it didn't parse correctly.
   }

   TEST( Shell, ParseChangeDirectory ) {
      change_directory_action *chdir;
      std::string expected_dir;

      expected_dir = "tmp";

      chdir = ParseChangeDirectoryAction("cd tmp");

      EXPECT_EQ( expected_dir, chdir->new_directory );

      expected_dir = "tmp";

      chdir = ParseChangeDirectoryAction("cd tmp ");

      EXPECT_EQ( expected_dir, chdir->new_directory );

      expected_dir = "tmp";

      chdir = ParseChangeDirectoryAction(" cd tmp");

      EXPECT_EQ( expected_dir, chdir->new_directory );

      expected_dir = "..";

      chdir = ParseChangeDirectoryAction("cd ..");

      EXPECT_EQ( expected_dir, chdir->new_directory );
   }

   TEST( Shell, ParseSingleCommandWithoutArguments ) {
      command *cmd;
      std::vector<std::string> expected_args;

      expected_args = { "foo" };

      cmd = ParseSingleCommand( "foo" );

      EXPECT_EQ( expected_args, cmd->args );

      cmd = ParseSingleCommand( " foo" );

      EXPECT_EQ( expected_args, cmd->args );

      cmd = ParseSingleCommand( "foo " );

      EXPECT_EQ( expected_args, cmd->args );

      cmd = ParseSingleCommand( " foo " );

      EXPECT_EQ( expected_args, cmd->args );

      cmd = ParseSingleCommand( "  foo  " );

      EXPECT_EQ( expected_args, cmd->args );
   }

   TEST( Shell, ParseSingleCommandWithArguments ) {
      command *cmd;
      std::vector<std::string> expected_args;

      expected_args = { "cmd", "1", "-n", "u" };

      cmd = ParseSingleCommand( "cmd 1 -n u" );

      EXPECT_EQ( expected_args, cmd->args );
   }

   TEST( Shell, ParseSingleCommandThatRunsInBackground ) {
      run_commands_action * cmdl;
      std::vector<std::string> expected_args;

      expected_args = { "foo" };

      cmdl = ParseRunCommandsAction( "foo &" );

      EXPECT_EQ( expected_args, cmdl->commands.front()->args );
      EXPECT_TRUE( cmdl->runInBackground );
   }

   TEST( Shell, ParseSingleCommandNoArgumentsWithRedirectStdin ) {
      command *cmd;
      std::vector<std::string> expected_args;
      std::string expected_input_file;

      expected_args = { "cmd" };
      expected_input_file = "inputfile";

      cmd = ParseSingleCommand( "cmd < inputfile" );

      EXPECT_EQ( expected_args, cmd->args );
      EXPECT_EQ( expected_input_file, cmd->input_file );
      EXPECT_EQ( "", cmd->output_file );
   }

   TEST( Shell, ParseSingleCommandWithRedirectStdin ) {
      command *cmd;
      std::vector<std::string> expected_args;
      std::string expected_input_file;

      expected_args = { "cmd", "arg1" };
      expected_input_file = "inputfile";

      cmd = ParseSingleCommand( "cmd arg1 < inputfile" );

      EXPECT_EQ( expected_args, cmd->args );
      EXPECT_EQ( expected_input_file, cmd->input_file );
      EXPECT_EQ( "", cmd->output_file );
   }

   TEST( Shell, ParseSingleCommandWithRedirectStdout ) {
      command *cmd;
      std::vector<std::string> expected_args;
      std::string expected_output_file;

      expected_args = { "cmd", "arg1" };
      expected_output_file = "outputfile";

      cmd = ParseSingleCommand( "cmd arg1 > outputfile" );

      EXPECT_EQ( expected_args, cmd->args );
      EXPECT_EQ( expected_output_file, cmd->output_file );
      EXPECT_EQ( "", cmd->input_file );
   }

   TEST( Shell, ParseSingleCommandWithRedirectStdinAndRedirectStdout ) {
      command *cmd;
      std::vector<std::string> expected_args;
      std::string expected_input_file, expected_output_file;

      expected_args = { "cmd", "arg1" };
      expected_input_file = "inputfile";
      expected_output_file = "outputfile";

      cmd = ParseSingleCommand( "cmd arg1 < inputfile > outputfile" );

      EXPECT_EQ( expected_args, cmd->args );
      EXPECT_EQ( expected_output_file, cmd->output_file );
      EXPECT_EQ( expected_input_file, cmd->input_file );
   }

   TEST( Shell, ParsePipeline ) {
      run_commands_action * cmdl;
      int expected_number_of_commands;
      std::vector< std::vector< std::string > > expected_args;

      expected_number_of_commands = 3;
      expected_args = { { "ls" "-la" }, { "sort" }, { "uniq -i" } };

      cmdl = ParseRunCommandsAction( "ls -la | sort | uniq -l" );

      EXPECT_EQ( expected_number_of_commands, cmdl->numberOfCommands );
      EXPECT_FALSE( cmdl->runInBackground );
   }

   TEST( Shell, PipelineThatRunsInBackground ) {
      run_commands_action * cmdl;

      cmdl = ParseRunCommandsAction( "cat < inputfile | sort | uniq &" );

      EXPECT_TRUE( cmdl->runInBackground );
   }

   TEST( Shell, ParsePipelineWithRedirectStdinAndRedirectStdout ) {
      run_commands_action * cmdl;
      int expected_number_of_commands;
      std::vector< std::vector< std::string > > expected_args;
      std::string expected_input_file, expected_output_file;

      expected_number_of_commands = 3;
      expected_args = { { "ls" "-la" }, { "sort" }, { "uniq -i" } };
      expected_input_file = "inputfile";
      expected_output_file = "outputfile";

      cmdl = ParseRunCommandsAction( "ls -la < inputfile | sort | uniq -l > outputfile" );

      EXPECT_EQ( expected_number_of_commands, cmdl->numberOfCommands );
      EXPECT_EQ( expected_input_file, cmdl->commands.front()->input_file );
      EXPECT_EQ( expected_output_file, cmdl->commands.back()->output_file );
   }

   TEST( Shell, ReadFromFile ) {
      Execute("cat < 1", "line 1\nline 2\nline 3\nline 4");
   }

   TEST( Shell, ReadFromAndWriteToFile ) {
      Execute("cat < 1 > ../foobar", "", "../foobar", "line 1\nline 2\nline 3\nline 4");
   }

   TEST( Shell, ReadFromAndWriteToFileChained ) {
      Execute("cat < 1 | head -n 3 > ../foobar", "", "../foobar", "line 1\nline 2\nline 3\n");
      Execute("cat < 1 | head -n 3 | tail -n 1 > ../foobar", "", "../foobar", "line 3\n");
   }

   TEST( Shell, WriteToFile ) {
      Execute("ls -1 > ../foobar", "", "../foobar", "1\n2\n3\n4\n");
   }

   TEST( Shell, Execute ) {
      Execute("uname", "Darwin\n");
      Execute("ls", "1\n2\n3\n4\n");
      Execute("ls -1", "1\n2\n3\n4\n");
   }

   TEST( Shell, ExecuteChained ) {
      Execute("ls -1 | head -n 2", "1\n2\n");
      Execute("ls -1 | head -n 2 | tail -n 1", "2\n");
   }

   TEST( Shell, ExecuteInBackground ) {
      Execute( "ls &", "" );
   }

   TEST( Shell, ChangeDirectory ) {
      system( "mkdir ../test-dir/nested" );
      Execute( "cd nested", "" );
      system( "rm -rf ../test-dir/nested" );

      Execute("cd this-directory-doesnt-exist", "No such file or directory");
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

   void Execute(std::string command, std::string expectedOutput) {
      filewrite("input", command);
      system("cd ../test-dir; " SHELL " < ../build/input > ../build/output 2> /dev/null");
      std::string got = filecontents("output");
      EXPECT_EQ(expectedOutput, got);
   }

   void Execute(std::string command, std::string expectedOutput, std::string expectedOutputFile, std::string expectedOutputFileContent) {
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

   nop_action* ParseNopAction( std::string input ) {
      shell_state state;

      parse_command( input, state );

      if ( nop_action * nop = static_cast< nop_action* >( state.action ) ) {
         return nop;
      } 
      else
         EXPECT_TRUE( false );

      return NULL;
   }

   exit_action* ParseExitAction( std::string input ) {
      shell_state state;

      parse_command( input, state );

      if ( exit_action * exit = static_cast< exit_action* >( state.action ) ) {
         return exit;
      } 
      else
         EXPECT_TRUE( false );

      return NULL;
   }

   change_directory_action* ParseChangeDirectoryAction( std::string input ) {
      shell_state state;

      parse_command( input, state );

      if ( change_directory_action * change_dir = static_cast< change_directory_action* >( state.action ) ) {
         return change_dir;
      } 
      else
         EXPECT_TRUE( false );

      return NULL;
   }

   run_commands_action* ParseRunCommandsAction( std::string input ) {
      shell_state state;

      parse_command( input, state );

      if ( run_commands_action * run_commands = static_cast< run_commands_action* >( state.action ) ) {
         return run_commands;
      } 

      EXPECT_TRUE( false );

      return NULL;
   }

   command* ParseSingleCommand( std::string input ) {
      run_commands_action * run_commands = ParseRunCommandsAction( input );

      EXPECT_EQ( 1, run_commands->commands.size() );

      return run_commands->commands.front();
   }
}
