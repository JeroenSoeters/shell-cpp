namespace shell { 
   extern int run_shell( bool prompt ); 
}

int main( int argc, char** argv ) {
    bool show_prompt = argc == 1;

    return shell::run_shell( show_prompt );
}
