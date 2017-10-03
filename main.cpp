extern int run_shell(bool prompt);

int main(int argc, char** argv) {
    bool showPrompt = argc == 1;
    return run_shell(showPrompt);
}
